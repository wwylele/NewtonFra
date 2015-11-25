/*
ExpreesionTree.cpp
表达式处理模块
*/

#include "main.h"
#include <stack>

//树节点类型
class TreeNode:public Token{
public:
	TreeNode* left,*right;
	TreeNode(const Token& t,TreeNode* l=nullptr,TreeNode* r=nullptr):
		Token(t),left(l),right(r){}
};


//复制树
TreeNode* CloneTree(TreeNode* node){
	TreeNode* p=new TreeNode(*node);

	//将树枝也进行复制
	if(p->left)p->left=CloneTree(p->left);
	if(p->right)p->right=CloneTree(p->right);
	return p;
}

//删除树
void DeleteTree(TreeNode* node){
	//删除树枝
	if(node->left)DeleteTree(node->left);
	if(node->right)DeleteTree(node->right);

	//删除自己
	delete node;
}




//树求导
void TreeDerivative(TreeNode* node,
	bool dy){
	switch(node->type){
	case 'c':
		//c' = 0
		node->value=0.0;
		break;
	//x' = 1
	case 'x':
		node->type='c';
		if(dy)
			node->value=0.0;
		else
			node->value=1.0;
		break;
	case 'y':
		node->type='c';
		if(!dy)
			node->value=0.0;
		else
			node->value=1.0;
		break;
	case 'f':
		
		if(node->pFunction==cfNeg){
			//(-u)' = -(u')
			TreeDerivative(node->left,dy);
		}
		else{
			//[f(u)]' = f'(u) * u'
			node->pFunction=cfDerivateMap.at(node->pFunction);
			TreeNode *l,*r;
			l=CloneTree(node);
			r=node->left;
			*node=TreeNode(Token(coMul),l,r);
			TreeDerivative(node->right,dy);
		}
		break;
	case 'o':
		if(node->pOperator==coAdd || node->pOperator==coSub){
			//(u+v)' = u' + v'
			TreeDerivative(node->left,dy);
			TreeDerivative(node->right,dy);
		}
		else if(node->pOperator==coMul){
			//(uv)' = u'v + uv'
			TreeNode *l,*r;
			l=new TreeNode(Token(coMul),node->left,node->right);
			r=new TreeNode(Token(coMul),CloneTree(node->left),CloneTree(node->right));
			*node=TreeNode(Token(coAdd),l,r);
			TreeDerivative(node->left->left,dy);
			TreeDerivative(node->right->right,dy);
		}
		else if(node->pOperator==coDiv){
			//(u/v)' = (u'v - uv')/v^2
			TreeNode *l,*r;
			l=new TreeNode(Token(coMul),node->left,node->right);
			r=new TreeNode(Token(coMul),CloneTree(node->left),CloneTree(node->right));
			*node=TreeNode(Token(coSub),l,r);
			TreeDerivative(node->left->left,dy);
			TreeDerivative(node->right->right,dy);
			*node=TreeNode(Token(coDiv),new TreeNode(*node),
				new TreeNode(Token(_cfSquare),CloneTree(l->right)));
		}
		else if(node->pOperator==coPow){
			if(node->right->type=='c'){
				//(u^c)' = c * (u' * u^(c-1))
				TreeNode *l,*r;
				l=new TreeNode(*node->right);
				r=new TreeNode(*node);
				*node=TreeNode(Token(coMul),l,r);
				l=CloneTree(node->right->left);
				TreeDerivative(l,dy);
				r=new TreeNode(*node->right);
				*node->right=TreeNode(Token(coMul),l,r);
				node->right->right->right->value-=1.0;
			}
			else if(node->left->type=='c'){
				//(c^u)' = ln(c) * (u' * c^u)
				TreeNode *l,*r;
				l=new TreeNode(Token(cfLog(node->left->value)));
				r=new TreeNode(*node);
				*node=TreeNode(Token(coMul),l,r);
				l=CloneTree(node->right->right);
				TreeDerivative(l,dy);
				r=new TreeNode(*node->right);
				*(node->right)=TreeNode(Token(coMul),l,r);
			}
			else{
				//(u^v)' = (u^v) * (v'*ln(u) + u'*v/u)
				TreeNode* u1,*u2,*du,*v,*dv;
				u1=CloneTree(node->left);
				u2=CloneTree(node->left);
				TreeDerivative(du=CloneTree(node->left),dy);
				v=CloneTree(node->right);
				TreeDerivative(dv=CloneTree(node->right),dy);
				TreeNode* logu=new TreeNode(Token(cfLog),u1),
					*dv_logu=new TreeNode(Token(coMul),dv,logu),
					*du_v=new TreeNode(Token(coMul),du,v),
					*du_v_div_u=new TreeNode(Token(coDiv),du_v,u2),
					*r=new TreeNode(Token(coAdd),dv_logu,du_v_div_u),
					*l=new TreeNode(*node);
				*node=TreeNode(Token(coMul),l,r);
			}
		}
		else throw;
		break;
	}
}

//树转换为后缀表达式 -需提前手动清空out
void TreeToPostfix(TreeNode* node,Postfix& out){
	switch(node->type){
	case 'c':
	case 'x':
	case 'y':
		out.emplace_back(*node);
		break;
	case 'f':
		TreeToPostfix(node->left,out);
		out.emplace_back(*node);
		break;
	case 'o':
		TreeToPostfix(node->left,out);
		TreeToPostfix(node->right,out);
		out.emplace_back(*node);
		break;
	}

}
//化简树
void SimplifyTree(TreeNode* node){
	static Complex O(0.0,0.0),I(1.0,0.0);
	switch(node->type){
	case 'c':
	case 'x':
	case 'y':
		break;
	case 'f':
		//将函数输入值化简
		SimplifyTree(node->left);
		//如果函数参数是常量则直接计算
		if(node->left->type=='c'){
			TreeNode *l=node->left;
			*node=TreeNode(node->pFunction(l->value));
			delete l;
		}
		break;
	case 'o':
		//先化简左右树枝
		SimplifyTree(node->left);
		SimplifyTree(node->right);
		if(node->left->type=='c' && node->right->type=='c'){
			//左右树枝都是常量则直接计算
			TreeNode *l,*r;
			l=node->left;
			r=node->right;
			*node=TreeNode(node->pOperator(l->value,r->value));
			delete l;
			delete r;
		}
		else if(node->pOperator==coAdd){//加法的化简
			if(node->left->type=='c' && node->left->value==O){
				//0+u=u
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=*r;
				delete l;
				delete r;
			}
			else if(node->right->type=='c' && node->right->value==O){
				//u+o=u
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=*l;
				delete l;
				delete r;
			}
		}
		else if(node->pOperator==coSub){//减法的化简
			if(node->left->type=='c' && node->left->value==O){
				//0-u=-u
				TreeNode  *l,*r;
				l=node->left;
				r=node->right;
				*node=TreeNode(Token(cfNeg),r);
				delete l;
			}
			else if(node->right->type=='c' && node->right->value==O){
				//u-0=u
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=*l;
				delete l;
				delete r;
			}
		}
		else if(node->pOperator==coMul){//乘法的化简
			if((node->left->type=='c' && node->left->value==O)||
				(node->right->type=='c' && node->right->value==O)){
				//u*0=0*u=0
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=TreeNode(Token(O));
				delete l;
				delete r;
			}
			else if(node->left->type=='c' && node->left->value==I){
				//1*u=u
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=*r;
				delete l;
				delete r;
			}
			else if(node->right->type=='c' && node->right->value==I){
				//u*1=u
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=*l;
				delete l;
				delete r;
			}
		}
		else if(node->pOperator==coDiv){//除法的化简
			if(node->left->type=='c' && node->left->value==O){
				//0/u=0
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=TreeNode(Token(O));
				delete l;
				delete r;
			}
			else if(node->right->type=='c' && node->right->value==I){
				//u/1=u
				TreeNode *l,*r;
				l=node->left;
				r=node->right;
				*node=*l;
				delete l;
				delete r;
			}
		}
		break;
	}
}

//树转换为中缀表达式字符串 -需提前手动清空out
void TreeToInfix(TreeNode* node,tstring* out,bool real=false){
	tstringsrteam ss;
	tstring buf;
	int myPrec;
	switch(node->type){
	case 'x':
		*out=_T("x");
		break;
	case 'y':
		*out=_T("y");
		break;
	case 'c':
		if(real || node->value.imag()==0.0){
			ss<<node->value.real();
		}
		else{
			ss<<_T("(")<<node->value.real()<<_T("+")<<node->value.imag()<<_T("*i)");
		}
		
		*out=ss.str();
		break;
	case 'f':
		TreeToInfix(node->left,&buf);
		for(auto pair:cfMap){
			if(pair.second==node->pFunction){
				ss<<pair.first;
				goto sucf;
			}
		}
		for(auto pair:_cfMap){
			if(pair.second==node->pFunction){
				ss<<pair.first;
				goto sucf;
			}
		}
		ss<<_T("<f>");
		sucf:
		ss<<_T("(")<<buf<<_T(")");
		*out=ss.str();
		break;
	case 'o':
		myPrec=OpPreced(node->pOperator);
		TreeToInfix(node->left,&buf);
		if(node->left->type=='c'||
			node->left->type=='x'||
			node->left->type=='y'||
			node->left->type=='f'||
			(node->left->type=='o'&&OpPreced(node->left->pOperator)>=myPrec)){
			ss<<buf;
		}
		else{
			ss<<_T("(")<<buf<<_T(")");
		}
		
		for(auto pair:coMap){
			if(pair.second==node->pOperator){
				ss<<pair.first;
				goto suco;
			}
		}
		ss<<_T("<o>");
		suco:
		TreeToInfix(node->right,&buf);
		if(node->right->type=='c'||
			node->right->type=='x'||
			node->right->type=='y'||
			node->right->type=='f'||
			(node->right->type=='o'&&OpPreced(node->right->pOperator)>myPrec)){
			ss<<buf;
		}
		else{
			ss<<_T("(")<<buf<<_T(")");
		}
		*out=ss.str();
		break;
	}
}

//根据后缀表达式构建树
TreeNode* BuildTree(const Postfix& in){
	std::stack<TreeNode*> nodeStack;

	//Build tree
	for(auto iter=in.begin();iter!=in.end();iter++){
		TreeNode* p;
		switch(iter->type){
		case 'c':case 'x':case 'y':nodeStack.push(new TreeNode(*iter));break;
		case 'f':nodeStack.top()=new TreeNode(*iter,nodeStack.top());break;
		case 'o':
			p=nodeStack.top();
			nodeStack.pop();
			nodeStack.top()=new TreeNode(*iter,nodeStack.top(),p);
			break;
		}
	}

	TreeNode* treeRoot=nodeStack.top();
	nodeStack.pop();
	if(!nodeStack.empty())throw;
	return treeRoot;
}

//表达式总处理-化简,求导,求中缀表达式
void FunctionProcessing(Postfix in[/*2*/]/*Postfix& in,Postfix& out*/,tstring out_infix[/*2*/]){


	TreeNode* treeRoot=BuildTree(in[0]);


	SimplifyTree(treeRoot);
	in[0].clear();
	TreeToPostfix(treeRoot,in[0]);
	TreeToInfix(treeRoot,out_infix);

	//Calculate derivate
	TreeDerivative(treeRoot,false);

	SimplifyTree(treeRoot);

	TreeToInfix(treeRoot,out_infix+1);

	//to Postfix
	in[1].clear();
	TreeToPostfix(treeRoot,in[1]);

	//clear
	DeleteTree(treeRoot);

	
}

//测试后缀表达式的栈深
int TestStackDepth(const Postfix& in){
	int depth=0,maxdepth=0;
	for(auto iter=in.begin();iter!=in.end();iter++){
		switch(iter->type){
		case 'c':case 'x':case 'y':depth++;if(depth>maxdepth)maxdepth=depth;break;
		case 'f':
			break;
		case 'o':
			depth--;
			break;
		}
	}
	return maxdepth;
}
int TestStackDepthSE(const PostfixSE& in){
	int depth=0,maxdepth=0;
	for(auto iter=in.begin();iter!=in.end();iter++){
		switch(iter->type){
		case 'c':case 'x':case 'y':depth++;if(depth>maxdepth)maxdepth=depth;break;
		case 'f':
			break;
		case 'o':
			depth--;
			break;
		}
	}
	return maxdepth;
}

//后缀表达式复数式与实数式相互转化
void RealizePostfix(const Postfix& in,PostfixSE& out){
	out.clear();
	for(auto iter=in.begin();iter!=in.end();iter++){
		switch(iter->type){
		case 'c':
			out.emplace_back(iter->value.real());
			break;
		case 'x':
			out.emplace_back();
			break;
		case 'y':
			out.emplace_back(true);
			break;
		case 'o':
			out.emplace_back(roMap.at(iter->pOperator));
			break;
		case 'f':
			out.emplace_back(rfMap.at(iter->pFunction));
			break;
		}
	}
}

void ComplexizePostfix(const PostfixSE& in,Postfix& out){
	out.clear();
	for(auto iter=in.begin();iter!=in.end();iter++){
		switch(iter->type){
		case 'c':
			out.emplace_back(Complex(iter->value));
			break;
		case 'x':
			out.emplace_back();
			break;
		case 'y':
			out.emplace_back(true);
			break;
		case 'o':
			for(auto pair:roMap){
				if(pair.second==iter->pOperator){
					out.emplace_back(pair.first);
					break;
				}
			}
			break;
		case 'f':
			for(auto pair:rfMap){
				if(pair.second==iter->pFunction){
					out.emplace_back(pair.first);
					break;
				}
			}
			break;
		}
	}
}

//表达式总处理-化简,求导,求中缀表达式-方程组模式
void FunctionProcessingSE(PostfixSE inout[/*6*/],tstring out_infix[/*6*/]){
	Postfix fx,fy;
	ComplexizePostfix(inout[0],fx);
	ComplexizePostfix(inout[1],fy);
	TreeNode* rootX,*rootY;
	rootX=BuildTree(fx);
	rootY=BuildTree(fy);
	SimplifyTree(rootX);
	SimplifyTree(rootY);
	fx.clear();
	fy.clear();
	TreeToInfix(rootX,out_infix+0,true);
	TreeToInfix(rootY,out_infix+1,true);
	TreeToPostfix(rootX,fx);
	TreeToPostfix(rootY,fy);
	RealizePostfix(fx,inout[0]);
	RealizePostfix(fy,inout[1]);
	TreeNode* rootDfxDx=rootX,*rootDfxDy,*rootDfyDx=rootY,*rootDfyDy;
	rootDfxDy=CloneTree(rootDfxDx);
	rootDfyDy=CloneTree(rootDfyDx);
	TreeDerivative(rootDfxDx,false);
	TreeDerivative(rootDfxDy,true);
	TreeDerivative(rootDfyDx,false);
	TreeDerivative(rootDfyDy,true);
	SimplifyTree(rootDfxDx);
	SimplifyTree(rootDfxDy);
	SimplifyTree(rootDfyDx);
	SimplifyTree(rootDfyDy);
	Postfix DfxDx,DfxDy,DfyDx,DfyDy;
	TreeToInfix(rootDfxDx,out_infix+2,true);
	TreeToInfix(rootDfxDy,out_infix+3,true);
	TreeToInfix(rootDfyDx,out_infix+4,true);
	TreeToInfix(rootDfyDy,out_infix+5,true);
	TreeToPostfix(rootDfxDx,DfxDx);
	TreeToPostfix(rootDfxDy,DfxDy);
	TreeToPostfix(rootDfyDx,DfyDx);
	TreeToPostfix(rootDfyDy,DfyDy);
	DeleteTree(rootDfxDx);
	DeleteTree(rootDfxDy);
	DeleteTree(rootDfyDx);
	DeleteTree(rootDfyDy);
	RealizePostfix(DfxDx,inout[2]);
	RealizePostfix(DfxDy,inout[3]);
	RealizePostfix(DfyDx,inout[4]);
	RealizePostfix(DfyDy,inout[5]);
}