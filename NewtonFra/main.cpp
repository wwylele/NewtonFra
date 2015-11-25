
/*
main.cpp
主框架
*/

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#undef max
#undef min
#include <algorithm>
#include "main.h"
#include "favorite.h"
#include "resource.h"



//绘图计时器ID
#define ID_TIMER_PAINT 123

#define _sub_t //-flag for functions that are only used by sub thread 
#define _mas_t //-flag for functions that are used by main thread and by sub thread

HINSTANCE hInstance;//实例句柄
HWND hMainWnd;//主窗口句柄
HWND hWndFavorite;//收藏窗口句柄
HANDLE hThread;//副线程句柄

//控制台相关
HWND hControlBox;//控制台窗口句柄
HWND hEditCoord;//鼠标坐标文本框句柄
HWND hStaticCpu;//CPU文本框句柄

//控制台程序操作标识符
//程序中修改控制台中的控件内容时，请将该项置为true
bool EditChangeBySoftware=false;

//窗口客户区尺寸 画布尺寸
//初始值为窗口初始尺寸
volatile int width=1024,height=768;

//画布 - 尺寸为width*height
BYTE* volatile canvas=nullptr;//画布 - 由副线程进行赋值 - 用于主线程的直接绘制
Complex* rawCanvas=nullptr;//复数画布 - 由副线程进行赋值 - 用于主线程的轮廓画布的计算
BYTE* outlineCanvas=nullptr;//轮廓画布 - 由主线程在绘制前赋值


//坐标相关
volatile double LTx,LTy;//画布左上角对应的分形坐标
volatile double RTx;//画布右上角对应的分形和坐标

//绘图参数
volatile double radii2=0.00000001;//收敛距离平方
volatile double escapeRadii2=1e100;//逃逸半径平方
volatile int iterCount=1000;//最大迭代次数
volatile bool showRoot=true;//标记零点
volatile bool outline=false;//绘制轮廓
volatile double outlineThickness=60.0;//轮廓浓度
Complex interf(1.0);//偏移

//线程同步相关
volatile bool calcFlag;//副线程正在运算时将此标志置于true，使主线程重新绘制
volatile bool stopFlag=false;//主线程将此标志置于true使副线程停止运算
HANDLE hEventStart;//主线程设置该事件使副线程开始运算
volatile bool presentFlag=false;//该标志为true使主线程重新绘制

//复变函数模式运算用栈
Complex *volatile stack=nullptr;//副线程使用
Complex *volatile stack2=nullptr;//主线程使用
//复变函数模式运算用函数
Postfix destFunction[2];
/*
[0]=F
[1]=dF/dx
*/
//复变函数模式运算用函数 - 中缀表达式
tstring destFunctionInfix[2];



//方程组模式相关
bool modeSE=false;//方程组模式开启标志
//方程组模式运算用栈
double*volatile stackSE=nullptr;//副线程使用
double*volatile stackSE2=nullptr;//主线程使用
//方程组模式运算用函数
PostfixSE destFunctionSE[6];
/*
[0]=Fx
[1]=Fy
[2]=dFx/dx
[3]=dFx/dy
[4]=dFy/dx
[5]=dFy/dy
*/
//方程组模式运算用函数 - 中缀表达式
tstring destFunctionInfixSE[6];


double ox,oy;//鼠标的分形坐标
bool trackSequence=true;//显示点列

FavoriteStore favorite;//收藏夹

//运算用函数初始化
void DefaultDestFunction(){
	destFunction[0]=Postfix{
		Token(),Token(),Token(),Token(coMul),Token(coMul),
		Token(Complex(1.0)),Token(coAdd)
	};

	destFunctionSE[0]=PostfixSE{
		TokenSE(),TokenSE(rfSin),TokenSE(true),TokenSE(rfTan),TokenSE(roSub)
	};
	destFunctionSE[1]=PostfixSE{
		TokenSE(true),TokenSE(rfCos),TokenSE(),TokenSE(rfTan),TokenSE(roSub)
	};
}

//主线程使用该函数停止副线程运算
void StopCalc(){
	if(calcFlag){
		int tic=iterCount;
		stopFlag=true;
		iterCount=0;
		while(calcFlag);
		iterCount=tic;
		stopFlag=false;

	}
}

//主线程使用该函数启动副线程运算
void StartCalc(){
	if(!calcFlag){
		SetEvent(hEventStart);
		while(!calcFlag);
	}
}

//显示报错框
void ErrBox(tstring str){
	MessageBox(hControlBox,str.c_str(),_T("错误"),0);
}

//重置坐标
void ResetAxis(){
	LTx=-2.1,LTy=1.5;
	RTx=2.0;
}

//对复数进行一次分形迭代
_mas_t inline void DoIter(const Complex& z,Complex& zn,Complex* stack/*使用的栈*/){
	int stackDepth=0;
	for(int f=0;f<2;f++){
		for(auto iter=destFunction[f].begin();iter!=destFunction[f].end();iter++){
			switch(iter->type){
			case 'c':stack[stackDepth++]=iter->value;break;
			case 'x':stack[stackDepth++]=z;break;
			case 'f':
				stack[stackDepth-1]=iter->pFunction(stack[stackDepth-1]);break;
			case 'o':
				stack[stackDepth-2]=iter->pOperator(stack[stackDepth-2],stack[stackDepth-1]);
				stackDepth--;
				break;
			}
		}
	}
	zn=stack[0]/stack[1]*interf;
}

//对坐标进行一次分形迭代
_mas_t inline void DoIterSE(double px,double py,double& xdp,double& ydp,double* stackSE/*使用的栈*/){
	int stackDepth=0;
	for(int f=0;f<6;f++){
		for(auto iter=destFunctionSE[f].begin();iter!=destFunctionSE[f].end();iter++){
			switch(iter->type){
			case 'c':stackSE[stackDepth++]=iter->value;break;
			case 'x':stackSE[stackDepth++]=px;break;
			case 'y':stackSE[stackDepth++]=py;break;
			case 'f':
				stackSE[stackDepth-1]=iter->pFunction(stackSE[stackDepth-1]);break;
			case 'o':
				stackSE[stackDepth-2]=iter->pOperator(stackSE[stackDepth-2],stackSE[stackDepth-1]);
				stackDepth--;
				break;
			}
		}
	}
	double det,xd,yd;
	det=stackSE[2]*stackSE[5]-stackSE[3]*stackSE[4];
	xd=(stackSE[0]*stackSE[5]-stackSE[3]*stackSE[1])/det;
	yd=(stackSE[2]*stackSE[1]-stackSE[0]*stackSE[4])/det;
	xdp=xd*interf.real()-yd*interf.imag();
	ydp=yd*interf.real()+xd*interf.imag();
	
}

//窗口坐标与分形坐标的相互转换
_mas_t inline void TranslatePoint(int x,int y,double& px,double &py){
	px=LTx+(RTx-LTx)*x/width;
	py=LTy-(RTx-LTx)*y/width;
}
_mas_t inline void TranslatePoint(double px,double py,int& x,int& y){
	x=int((px-LTx)*width/(RTx-LTx));
	y=int((-py+LTy)*width/(RTx-LTx));
}

//计算给定像素的颜色值和复数值，并具有检测stopFlag的功能
_sub_t inline bool/*被StopCalc中断则返回false*/ 
CalcPixel(int x,int y,BYTE& r,BYTE& g,BYTE& b,Complex& c){
	static bool uncert=false;
	uncert=!uncert;
	if(stopFlag)return false;
	double px,py;
	TranslatePoint(x,y,px,py);
	int i;

	Complex z(px,py),z0;
	z0=z;
	if(modeSE){
		for(i=uncert?0:-1;i<iterCount;i++){
			double xdp,ydp;
			DoIterSE(px,py,xdp,ydp,stackSE);
			
			if(xdp*xdp+ydp*ydp<=radii2){
				break;
			}
			px-=xdp;
			py-=ydp;
			if(px*px+py*py>=escapeRadii2){
				r=g=b=0;
				c=Complex(HUGE_VALF);
				return true;
			}
		}
		z=Complex(px,py);
	}
	else{
		Complex zn;
		
		for(i=uncert?0:-1;i<iterCount;i++){
			DoIter(z,zn,stack);
			if(std::norm(zn)<=radii2){
				break;
			}
			z-=zn;
			if(std::norm(z)>=escapeRadii2){
				r=g=b=0;
				c=Complex(HUGE_VALF);
				return true;
			}
		}
	}
	


	c=z;

	static const double a=200.0,sq=sqrt(3.0)*0.5;
	int ir,ig,ib;
	ir=(int)((z.real()-1.0)*153.2+22.2)%512;
	ig=(int)((z.real()*0.5-z.imag()*sq+1.0)*236.9-343.2)%512;
	ib=(int)((z.real()*0.5+z.imag()*sq+1.0)*178.4)%512;
	if(ir<0)ir+=511;
	if(ig<0)ig+=511;
	if(ib<0)ib+=511;
	r=ir<256?ir:511-ir;
	g=ig<256?ig:511-ig;
	b=ib<256?ib:511-ib;

	if(showRoot){
		double rootradii=8*(RTx-LTx)/width;
		double q=std::norm(z0-z)/(rootradii*rootradii);
		if(q<1.0){
			r=g=b=(BYTE)(255*(1.0-q));
			//g=b=0;
		}
	}

	return true;
}

//为画布（轮廓画布除外）设置像素
_sub_t inline void SetPixel(int x,int y,BYTE r,BYTE g,BYTE b,const Complex& c){
	if(x>=width || y>=height)return;

	rawCanvas[x+y*width]=c;


	int p=(x+y*width)*3;
	canvas[p+2]=r;
	canvas[p+1]=g;
	canvas[p]=b;
	

	
}

//第一轮计算渲染
/*
 # # # # 
 # # # #
 # # # #
 # # # #
*/
_sub_t bool/*被StopCalc中断则返回false*/ FirstRender(int step/*步长*/){
	BYTE r,g,b;
	Complex c;
	for(int x=0;x<width;x+=step){
		for(int y=0;y<height;y+=step){
			if(!CalcPixel(x,y,r,g,b,c)){
				return false;
			}
			for(int tx=0;tx<step;tx++)for(int ty=0;ty<step;ty++)
			{
				SetPixel(x+tx,y+ty,r,g,b,c);
			}

		}
	}
	return true;
}

//一轮计算渲染
/*
   #   #
 # # # #
   #   #
 # # # #
*/
_sub_t bool/*被StopCalc中断则返回false*/ NextRender(int step/*步长*/){
	BYTE r,g,b;
	Complex c;
	bool bx=false,by=false;
	for(int x=0;x<width;x+=step){
		by=false;
		for(int y=0;y<height;y+=step){

			if(bx||by){
				if(!CalcPixel(x,y,r,g,b,c)){
					return false;
				}
				for(int tx=0;tx<step;tx++)for(int ty=0;ty<step;ty++)
				{
					SetPixel(x+tx,y+ty,r,g,b,c);
				}
			}
			by=!by;
		}
		bx=!bx;
	}
	return true;
}

//开始所有计算渲染
_sub_t bool/*被StopCalc中断则返回false*/ BeginAllRender(){
	int step=64;
	if(!FirstRender(step))return false;
	step>>=1;
	while(1){
		if(!NextRender(step))return false;
		if(step==1)break;
		step>>=1;
	}
	return true;
}

//副线程入口点
_sub_t DWORD WINAPI ThreadCalc(LPVOID){
	while(1){
		calcFlag=false;
		presentFlag=true;
		WaitForSingleObject(hEventStart,INFINITE);
		calcFlag=true;
		BeginAllRender();
	}


	return 0;
}

//调整尺寸
void ResizeCanvas(int newWidth,int newHeight){
	width=newWidth+(4-newWidth%4);
	height=newHeight+(4-newHeight%4);
	if(canvas)delete[] canvas;
	if(outlineCanvas)delete[] outlineCanvas;
	if(rawCanvas)delete[] rawCanvas;
	canvas=new BYTE[width*height*3];
	outlineCanvas=new BYTE[width*height*3];
	rawCanvas=new Complex[width*height];
}

//绘制
void Paint(HDC hDC){

	if(outline){
		//Paint outline
		double s;
		Complex* p;
		BYTE *p2;
		for(int y=1;y<height-1;y++)for(int x=1;x<width-1;x++){
			s=0;
			p=rawCanvas+x+y*width;
			s+=std::norm(*p-*(p-1));
			s+=std::norm(*p-*(p+1));
			s+=std::norm(*p-*(p-width));
			s+=std::norm(*p-*(p+width));

			p2=outlineCanvas+(x+y*width)*3;
			s*=outlineThickness;
			*(p2+2)=*(p2+1)=*p2=(BYTE)std::min({s,255.0});
			
		}
	}

	BITMAPINFO binfo;
	binfo.bmiHeader.biSize=sizeof(BITMAPINFO);
	binfo.bmiHeader.biWidth=width;
	binfo.bmiHeader.biHeight=-height;
	binfo.bmiHeader.biPlanes=1;
	binfo.bmiHeader.biBitCount=24;
	binfo.bmiHeader.biCompression=BI_RGB;
	binfo.bmiHeader.biSizeImage=0;
	binfo.bmiHeader.biClrUsed=0;
	binfo.bmiHeader.biClrImportant=0;


	StretchDIBits(hDC,0,0,width,height,0,0,width,height,
		outline?outlineCanvas:canvas,&binfo,DIB_RGB_COLORS,SRCCOPY);

	//Mouse Seq
	if(trackSequence){
		//SetROP2(hDC,R2_NOT);
		int iterCount=std::min({::iterCount,1000});
		if(modeSE){
			int x,y;
			TranslatePoint(ox,oy,x,y);
			MoveToEx(hDC,x,y,nullptr);
			double px,py;
			px=ox,py=oy;
			for(int i=0;i<iterCount;i++){
				double xdp,ydp;
				DoIterSE(px,py,xdp,ydp,stackSE2);

				if(xdp*xdp+ydp*ydp<=radii2){
					break;
				}
				px-=xdp;
				py-=ydp;
				TranslatePoint(px,py,x,y);
				LineTo(hDC,x,y);
				if(px*px+py*py>=escapeRadii2){
					break;
				}
			}
		}
		else{
			int x,y;
			TranslatePoint(ox,oy,x,y);
			MoveToEx(hDC,x,y,nullptr);
			Complex z(ox,oy),zn;

			for(int i=0;i<iterCount;i++){
				DoIter(z,zn,stack2);
				if(std::norm(zn)<=radii2){
					break;
				}
				z-=zn;
				TranslatePoint(z.real(),z.imag(),x,y);
				LineTo(hDC,x,y);
				if(std::norm(z)>=escapeRadii2){
					break;
				}
			}

		}
	}
	
}

//主窗口消息处理
LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	POINT pt;
	double dlx,dly,lam;
	int MouseWheel;
	static bool moving=false;
	static int lx,ly;
	TCHAR buf[50];
	switch(message)
	{
	case WM_LBUTTONDOWN:
		if(!moving){
			moving=true;
			lx=GET_X_LPARAM(lParam);
			ly=GET_Y_LPARAM(lParam);
			SetCapture(hWnd);
		}
		break;
	case WM_LBUTTONUP:
		if(moving){
			moving=false;
			ReleaseCapture();
		}
		break;

	case WM_MOUSEMOVE:
					  
		pt.x=GET_X_LPARAM(lParam);
		pt.y=GET_Y_LPARAM(lParam);
		TranslatePoint(pt.x,pt.y,ox,oy);
		TranslatePoint(lx,ly,dlx,dly);
		if(moving){
			StopCalc();
			LTx-=ox-dlx;
			RTx-=ox-dlx;
			LTy-=oy-dly;
			lx=pt.x;
			ly=pt.y;
			StartCalc();
		}
		TranslatePoint(pt.x,pt.y,ox,oy);
		if(trackSequence)presentFlag=true;
		
		_stprintf_s(buf,50,_T("(%lg,%lg)"),ox,oy);
		SetWindowText(hEditCoord,buf);
	

		break;
	case WM_MOUSEWHEEL:
		StopCalc();
		pt.x=GET_X_LPARAM(lParam);
		pt.y=GET_Y_LPARAM(lParam);
		ScreenToClient(hWnd,&pt);
		TranslatePoint(pt.x,pt.y,ox,oy);
		MouseWheel=GET_WHEEL_DELTA_WPARAM(wParam);
		lam=pow(2.0,-MouseWheel/300.0);
		LTx=(LTx-ox)*lam+ox;
		LTy=(LTy-oy)*lam+oy;
		RTx=(RTx-ox)*lam+ox;
		StartCalc();
		break;
	case WM_RBUTTONDOWN:
		StopCalc();
		ResetAxis();
		StartCalc();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		StopCalc();
		ResizeCanvas(LOWORD(lParam),HIWORD(lParam));
		StartCalc();
		break;
	case WM_TIMER:
		_itot(GetCpuPercentage(),buf,10);
		SetWindowText(hStaticCpu,buf);
		if(wParam==ID_TIMER_PAINT){
			if(presentFlag||calcFlag){
				presentFlag=false;
				HDC hDC;
				hDC=GetDC(hMainWnd);
				Paint(hDC);
				ReleaseDC(hMainWnd,hDC);
			}
		}
		break;
	default:
		return DefWindowProc(hWnd,message,wParam,lParam);
	}
	return 0;
}

//开辟运算用栈
void AllocStack(){
	int stackDepth;
	stackDepth=1+std::max({
		TestStackDepth(destFunction[0]),
		TestStackDepth(destFunction[1])
	});
	delete[] stack;
	delete[] stack2;
	stack=new Complex[stackDepth];
	stack2=new Complex[stackDepth];
}

//开辟运算用栈 - 复变函数模式
void AllocStackSE(){
	int stackDepth;
	stackDepth=5+std::max({
		TestStackDepthSE(destFunctionSE[0]),
		TestStackDepthSE(destFunctionSE[1]),
		TestStackDepthSE(destFunctionSE[2]),
		TestStackDepthSE(destFunctionSE[3]),
		TestStackDepthSE(destFunctionSE[4]),
		TestStackDepthSE(destFunctionSE[5]),
	});
	delete[] stackSE;
	delete[] stackSE2;
	stackSE=new double[stackDepth];
	stackSE2=new double[stackDepth];
}

//更新收藏列表
void UpdateFavoriteList(){
	HWND hList=GetDlgItem(hWndFavorite,IDC_LIST_FAVORITE);
	ListBox_ResetContent(hList);
	for(int i=0;i<favorite.GetFavoriteCount();i++){
		ListBox_AddString(hList,favorite.GetFavoriteTitle(i).c_str());
	}
}

//更新所有控制台参数
void UpdateConsoleData(HWND hDlg){
	EditChangeBySoftware=true;
	TCHAR buf[50];
	TabCtrl_SetCurSel(GetDlgItem(hDlg,IDC_TAB_MODE),modeSE?1:0);
	EnableWindow(GetDlgItem(hDlg,IDC_EDIT_INPUT_FUNCTION_Y),modeSE?TRUE:FALSE);

	_itot(iterCount,buf,10);
	SetDlgItemText(hDlg,IDC_STATIC_ITERCOUNT,buf);
	SetDlgItemText(hDlg,IDC_EDIT_ITERCOUNT,buf);

	_stprintf_s(buf,49,_T("%lg"),radii2);
	SetDlgItemText(hDlg,IDC_STATIC_RADII,buf);
	SetDlgItemText(hDlg,IDC_EDIT_RADII,buf);

	_stprintf_s(buf,49,_T("%lg"),escapeRadii2);
	SetDlgItemText(hDlg,IDC_STATIC_ESCAPE_RADII,buf);
	SetDlgItemText(hDlg,IDC_EDIT_ESCAPE_RADII,buf);

	_stprintf_s(buf,49,_T("%lg"),outlineThickness);
	SetDlgItemText(hDlg,IDC_EDIT_OUTLINE,buf);

	_stprintf_s(buf,49,_T("%lg+%lgi"),interf.real(),interf.imag());
	SetDlgItemText(hDlg,IDC_STATIC_INTERF,buf);
	_stprintf_s(buf,49,_T("%lg"),interf.real());
	SetDlgItemText(hDlg,IDC_EDIT_INTERF_X,buf);
	_stprintf_s(buf,49,_T("%lg"),interf.imag());
	SetDlgItemText(hDlg,IDC_EDIT_INTERF_Y,buf);

	SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
		destFunctionInfix[0].c_str());
	SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
		destFunctionInfix[1].c_str());

	CheckDlgButton(hDlg,IDC_CHECK_ROOT,showRoot?TRUE:FALSE);
	CheckDlgButton(hDlg,IDC_CHECK_TRACK,trackSequence?TRUE:FALSE);

	if(modeSE){
		SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
			(destFunctionInfixSE[0]+_T("\r\n")
			+destFunctionInfixSE[1]).c_str());
		SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
			(destFunctionInfixSE[2]+_T("\r\n")
			+destFunctionInfixSE[3]+_T("\r\n")
			+destFunctionInfixSE[4]+_T("\r\n")
			+destFunctionInfixSE[5]).c_str());
	}
	else{
		SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
			destFunctionInfix[0].c_str());
		SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
			destFunctionInfix[1].c_str());
	}
	EditChangeBySoftware=false;
}

//收藏夹窗口消息处理
INT_PTR CALLBACK WndFavoriteProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message){
	case WM_CLOSE:
		ShowWindow(hDlg,SW_HIDE);
		return TRUE;
	case WM_COMMAND:
		switch(wParam){
		case MAKEWPARAM(IDC_LIST_FAVORITE,LBN_SELCHANGE):{
			int fi;
			fi=ListBox_GetCurSel(GetDlgItem(hWndFavorite,IDC_LIST_FAVORITE));
			StopCalc();

			FavoriteStore::ArgPack arg;
			tstring dest[2];
			favorite.GetFavorite(fi,arg,dest);
			LTx=arg.LTx;
			LTy=arg.LTy;
			RTx=arg.RTx;
			escapeRadii2=arg.escapeRadii2;
			radii2=arg.radii2;
			iterCount=arg.iterCount;
			showRoot=arg.showRoot;
			outline=arg.outline;
			outlineThickness=arg.outlineThickness;
			interf=Complex(arg.interfX,arg.interfY);
			if(dest[1]==_T("")){
				modeSE=false;
				InfixStringToPostfix(dest[0].c_str(),destFunction[0]);
				FunctionProcessing(destFunction,destFunctionInfix);
				AllocStack();
			}
			else{
				modeSE=true;
				Postfix newDestFunction,newDestFunction2;
				InfixStringToPostfix(dest[0].c_str(),newDestFunction,true);
				InfixStringToPostfix(dest[1].c_str(),newDestFunction2,true);
				RealizePostfix(newDestFunction,destFunctionSE[0]);
				RealizePostfix(newDestFunction2,destFunctionSE[1]);
				FunctionProcessingSE(destFunctionSE,destFunctionInfixSE);
				AllocStackSE();
			}
			UpdateConsoleData(hControlBox);

			StartCalc();
			return TRUE;
		}
		default:
			return FALSE;
		}

	default:
		return FALSE;
	}
}

//控制台消息处理
INT_PTR CALLBACK CtrlBoxProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	TCHAR buf[50];
	switch(message){
	case WM_INITDIALOG:
		EditChangeBySoftware=true;
		hEditCoord=GetDlgItem(hDlg,IDC_EDIT_COORD);
		hStaticCpu=GetDlgItem(hDlg,IDC_STATIC_CPU);
		
		TC_ITEM ti;
		ti.mask=TCIF_TEXT;
		ti.pszText=_T("复变函数模式");
		TabCtrl_InsertItem(GetDlgItem(hDlg,IDC_TAB_MODE),0,&ti);
		ti.pszText=_T("方程组模式");
		TabCtrl_InsertItem(GetDlgItem(hDlg,IDC_TAB_MODE),1,&ti);

		UpdateConsoleData(hDlg);

		EditChangeBySoftware=false;
		return TRUE;
	case WM_MOUSEWHEEL:
		SendMessage(hMainWnd,message,wParam,lParam);
		return TRUE;
	case WM_NOTIFY:
		NMHDR *lpnmhdr;
		lpnmhdr=(LPNMHDR)lParam;
		switch(lpnmhdr->idFrom){
		case IDC_TAB_MODE:
			if(lpnmhdr->code==TCN_SELCHANGE){
				StopCalc();
				modeSE=(TabCtrl_GetCurSel(GetDlgItem(hDlg,IDC_TAB_MODE))?true:false);
				if(modeSE){
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
						(destFunctionInfixSE[0]+_T("\r\n")
						+destFunctionInfixSE[1]).c_str());
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
						(destFunctionInfixSE[2]+_T("\r\n")
						+destFunctionInfixSE[3]+_T("\r\n")
						+destFunctionInfixSE[4]+_T("\r\n")
						+destFunctionInfixSE[5]).c_str());
				}
				else{
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
						destFunctionInfix[0].c_str());
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
						destFunctionInfix[1].c_str());
				}
				StartCalc();
				EnableWindow(GetDlgItem(hDlg,IDC_EDIT_INPUT_FUNCTION_Y),modeSE?TRUE:FALSE);
			}
			return TRUE;
		default:
			return FALSE;
		}
	case WM_COMMAND:
		switch(wParam){
		case MAKEWPARAM(IDC_BUTTON_FAVORITE,BN_CLICKED):
			ShowWindow(hWndFavorite,SW_SHOW);
			SetFocus(hWndFavorite);
			return TRUE;
		case MAKEWPARAM(IDC_EDIT_OUTLINE,EN_CHANGE):
			if(!EditChangeBySoftware){
				GetDlgItemText(hDlg,IDC_EDIT_OUTLINE,buf,49);
				outlineThickness=_ttof(buf);
				presentFlag=true;
			}
			return TRUE;
		case MAKEWPARAM(IDC_EDIT_INTERF_X,EN_CHANGE):
		case MAKEWPARAM(IDC_EDIT_INTERF_Y,EN_CHANGE):
			if(!EditChangeBySoftware){
				double x,y;
				GetDlgItemText(hDlg,IDC_EDIT_INTERF_X,buf,49);
				x=_ttof(buf);
				GetDlgItemText(hDlg,IDC_EDIT_INTERF_Y,buf,49);
				y=_ttof(buf);
				Complex _interf(x,y);
				if(interf!=_interf){
					StopCalc();
					interf=_interf;
					StartCalc();
					_stprintf_s(buf,49,_T("%lg+%lgi"),interf.real(),interf.imag());
					SetDlgItemText(hDlg,IDC_STATIC_INTERF,buf);
				}

				
			}
			return TRUE;
		case MAKEWPARAM(IDC_CHECK_ROOT,BN_CLICKED):
			if(!EditChangeBySoftware){
				StopCalc();
				showRoot=Button_GetCheck(GetDlgItem(hDlg,IDC_CHECK_ROOT))==TRUE;
				StartCalc();
			}
			return TRUE;
		case MAKEWPARAM(IDC_CHECK_TRACK,BN_CLICKED):
			if(!EditChangeBySoftware){
				trackSequence=Button_GetCheck(GetDlgItem(hDlg,IDC_CHECK_TRACK))==TRUE;
				presentFlag=true;
			}
			return TRUE;
		case MAKEWPARAM(IDC_CHECK_OUTLINE,BN_CLICKED):
			if(!EditChangeBySoftware){
				outline=Button_GetCheck(GetDlgItem(hDlg,IDC_CHECK_OUTLINE))==TRUE;
				EnableWindow(GetDlgItem(hDlg,IDC_EDIT_OUTLINE),outline?TRUE:FALSE);
				presentFlag=true;
			}
			return TRUE;
		case MAKEWPARAM(IDC_BUTTON_INPUT_FUNCTION,BN_CLICKED):{
			int l;
			TCHAR *buf,*buf2;
			l=GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT_INPUT_FUNCTION));
			buf=new TCHAR[l+1];
			GetDlgItemText(hDlg,IDC_EDIT_INPUT_FUNCTION,buf,l+1);
			_tcslwr(buf);
			l=GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT_INPUT_FUNCTION_Y));
			buf2=new TCHAR[l+1];
			GetDlgItemText(hDlg,IDC_EDIT_INPUT_FUNCTION_Y,buf2,l+1);
			_tcslwr(buf2);

			if(modeSE){
				Postfix newDestFunction,newDestFunction2;
				if(InfixStringToPostfix(buf,newDestFunction,true) &&
					InfixStringToPostfix(buf2,newDestFunction2,true)){
					StopCalc();
					RealizePostfix(newDestFunction,destFunctionSE[0]);
					RealizePostfix(newDestFunction2,destFunctionSE[1]);
					FunctionProcessingSE(destFunctionSE,destFunctionInfixSE);
					AllocStackSE();
					ResetAxis();
					StartCalc();
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
						(destFunctionInfixSE[0]+_T("\r\n")
						+destFunctionInfixSE[1]).c_str());
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
						(destFunctionInfixSE[2]+_T("\r\n")
						+destFunctionInfixSE[3]+_T("\r\n")
						+destFunctionInfixSE[4]+_T("\r\n")
						+destFunctionInfixSE[5]).c_str());
				}
			}
			else{
				Postfix newDestFunction;
				if(InfixStringToPostfix(buf,newDestFunction)){
					StopCalc();
					destFunction[0]=newDestFunction;
					FunctionProcessing(destFunction,destFunctionInfix);
					AllocStack();
					ResetAxis();
					StartCalc();
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION0,
						destFunctionInfix[0].c_str());
					SetDlgItemText(hDlg,IDC_EDIT_DESTFUNCTION1,
						destFunctionInfix[1].c_str());
				}
			}
			
			delete[] buf;
			delete[] buf2;
			return TRUE;
		}
		case MAKEWPARAM(IDC_BUTTON_ADD_FAVORITE,BN_CLICKED):{
			int l;
			TCHAR* buf;
			l=GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT_FAVORITE));
			if(!l){
				MessageBox(hDlg,_T("请输入名称!"),_T("失败"),MB_ICONWARNING);
				SetFocus(GetDlgItem(hDlg,IDC_EDIT_FAVORITE));
			}
			else{

				buf=new TCHAR[l+1];
				GetDlgItemText(hDlg,IDC_EDIT_FAVORITE,buf,l+1);
				FavoriteStore::ArgPack arg;
				arg.LTx=LTx;
				arg.LTy=LTy;
				arg.RTx=RTx;
				arg.escapeRadii2=escapeRadii2;
				arg.radii2=radii2;
				arg.iterCount=iterCount;
				arg.showRoot=showRoot;
				arg.outline=outline;
				arg.outlineThickness=outlineThickness;
				arg.interfX=interf.real();
				arg.interfY=interf.imag();
				favorite.AddFavorite(arg,
					modeSE?destFunctionInfixSE[0]:destFunctionInfix[0],
					modeSE?destFunctionInfixSE[1]:_T(""),
					buf);
				UpdateFavoriteList();
				MessageBox(hDlg,_T("已加入收藏夹"),_T("成功"),0);
				delete[] buf;
				SetDlgItemText(hDlg,IDC_EDIT_FAVORITE,_T(""));
			}
			return TRUE;
		}
			
		case MAKEWPARAM(IDC_EDIT_ITERCOUNT,EN_CHANGE):
			if(!EditChangeBySoftware){
				int iterCount_;
				GetDlgItemText(hDlg,IDC_EDIT_ITERCOUNT,buf,49);
				iterCount_=std::min({std::max({_ttoi(buf),1}),1000000000});
				if(iterCount_!=iterCount)
				{
					StopCalc();
					iterCount=iterCount_;
					StartCalc();
					_itot(iterCount,buf,10);
					SetDlgItemText(hDlg,IDC_STATIC_ITERCOUNT,buf);
				}
			}

			return TRUE;
		case MAKEWPARAM(IDC_EDIT_RADII,EN_CHANGE):
			if(!EditChangeBySoftware){
				double radii_;
				GetDlgItemText(hDlg,IDC_EDIT_RADII,buf,49);
				radii_=abs(_ttof(buf));
				if(radii_!=radii2)
				{
					StopCalc();
					radii2=radii_;
					StartCalc();
					_stprintf_s(buf,49,_T("%lg"),radii2);
					SetDlgItemText(hDlg,IDC_STATIC_RADII,buf);
				}
			}
			return TRUE;
		case MAKEWPARAM(IDC_EDIT_ESCAPE_RADII,EN_CHANGE):
			if(!EditChangeBySoftware){
				double radii_;
				GetDlgItemText(hDlg,IDC_EDIT_ESCAPE_RADII,buf,49);
				radii_=abs(_ttof(buf));
				if(radii_!=escapeRadii2)
				{
					StopCalc();
					escapeRadii2=radii_;
					StartCalc();
					_stprintf_s(buf,49,_T("%lg"),escapeRadii2);
					SetDlgItemText(hDlg,IDC_STATIC_ESCAPE_RADII,buf);
				}
			}
			return TRUE;
		default:return FALSE;
		}
	case WM_CLOSE:
		PostQuitMessage(0);
		return TRUE;
	default:
		return FALSE;
	}
}

//初始化窗口相关
void InitWindow(int nShowCmd){
	const TCHAR* wndClassName=_T("NewtonFra");
	WNDCLASSEX wcex;

	wcex.cbSize=sizeof(WNDCLASSEX);
	wcex.style=0;
	wcex.lpfnWndProc=WndProc;
	wcex.cbClsExtra=0;
	wcex.cbWndExtra=0;
	wcex.hInstance=hInstance;
	wcex.hIcon=LoadIcon(hInstance,MAKEINTRESOURCE(IDI_MAIN));
	wcex.hCursor=LoadCursor(NULL,IDC_SIZEALL);
	wcex.hbrBackground=0;
	wcex.lpszMenuName=NULL;
	wcex.lpszClassName=wndClassName;
	wcex.hIconSm=LoadIcon(hInstance,MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassEx(&wcex);

	hMainWnd=CreateWindow(wndClassName,_T("Newton Fractal Painter"),
		WS_OVERLAPPEDWINDOW,
		0,0,width,height,NULL,NULL,hInstance,NULL);

	SetTimer(hMainWnd,ID_TIMER_PAINT,100,nullptr);

	hControlBox=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_CONTROL),hMainWnd,CtrlBoxProc);
	hWndFavorite=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_FAVORITE),hMainWnd,WndFavoriteProc);

	RECT rc;
	GetClientRect(hMainWnd,&rc);
	
	StopCalc();
	ResizeCanvas(rc.right-rc.left,rc.bottom-rc.top);
	ResetAxis();
	StartCalc();

	UpdateFavoriteList();

	ShowWindow(hControlBox,SW_SHOW);
	ShowWindow(hMainWnd,nShowCmd);
	UpdateWindow(hMainWnd);

}

//初始线程相关
void InitMultiThread(){
	hEventStart=CreateEvent(nullptr,FALSE,FALSE,nullptr);
	hThread=CreateThread(nullptr,0,ThreadCalc,nullptr,0,nullptr);
}

//主入口
int WINAPI _tWinMain(
	_In_ HINSTANCE hI,
	_In_opt_ HINSTANCE,
	_In_ LPTSTR lpCmdLine,
	_In_ int nShowCmd){
	hInstance=hI;

	DefaultDestFunction();

	FunctionProcessing(destFunction,destFunctionInfix);
	FunctionProcessingSE(destFunctionSE,destFunctionInfixSE);
	AllocStack();
	AllocStackSE();

	InitMultiThread();
	InitWindow(nShowCmd);
	

	MSG msg={0};
	while(GetMessage(&msg,0,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	}
	SuspendThread(hThread);
	delete[] canvas;
	delete[] outlineCanvas;
	delete[] rawCanvas;
	delete[] stack;
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return (int)msg.wParam;
}

//控件样式
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif