/*
main.h
主头文件
*/

#ifndef _MAIN_H_
#define _MAIN_H_
#include <tchar.h>
#include <math.h>
#include <complex>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#ifdef _DEBUG
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#define new DEBUG_NEW
#endif


using tstring=std::basic_string<TCHAR>;//可变字符串类型
using tstringsrteam=std::basic_stringstream<TCHAR>;//可变字符串流类型
using Complex=std::complex<double>;//复数类型
using ComplexFunction=Complex(*)(const Complex&);//复数函数类型
using ComplexOperator=Complex(*)(const Complex&,const Complex&);//复数运算符类型

//复数表达式元素
class Token{
public:
	char type;
	/* type的可用取值:
	'c' - 常量
	'x' - 自变量
	'y' - 第二自变量 //仅在方程组模式下出现
	'f' - 函数（一元运算符）
	'o' - 二元运算符
	（以下仅在中缀表达式中出现）
	'(' - 左括号
	')' - 右括号
	'_' - 负号临时标记
	*/
	Complex value;
	union{
		ComplexFunction pFunction;
		ComplexOperator pOperator;
	};
	Token():type('x'){}
	Token(bool dummy):type('y'){}
	Token(const Complex &v):type('c'),value(v){}
	Token(ComplexFunction cf):type('f'),pFunction(cf){}
	Token(ComplexOperator co):type('o'),pOperator(co){}
	Token(char mark):type(mark){}
};

//复数后缀表达式
using Postfix=std::list<Token>;

using RealFunction=double(*)(double);//实数函数类型
using RealOperator=double(*)(double,double);//实数运算符类型

//实数表达式元素
class TokenSE{
public:
	char type;//内容与Token::type相同
	
	double value;
	union{
		RealFunction pFunction;
		RealOperator pOperator;
	};
	TokenSE():type('x'){}
	TokenSE(bool dummy):type('y'){}
	TokenSE(double v):type('c'),value(v){}
	TokenSE(RealFunction cf):type('f'),pFunction(cf){}
	TokenSE(RealOperator co):type('o'),pOperator(co){}
	//TokenSE(char mark):type(mark){}
};

//实数后缀表达式
using PostfixSE=std::list<TokenSE>;

//基本函数的声明，可能是函数声明或函数指针
extern ComplexOperator coAdd,coSub,coMul,coDiv,coPow;
extern ComplexFunction 
cfSin,cfCos,cfTan,
cfAsin,cfAcos,cfAtan,
cfSinh,cfCosh,cfTanh,
cfAsinh,cfAcosh,cfAtanh,
cfExp,cfLog,
cfNeg;

extern ComplexFunction
_cfCosD,_cfTanD,
_cfAsinD,_cfAcosD,_cfAtanD,
_cfTanhD,
_cfAsinhD,_cfAcoshD,_cfAtanhD,
_cfInverse,_cfSquare;

double roAdd(double a,double b);
double roSub(double a,double b);
double roMul(double a,double b);
double roDiv(double a,double b);
extern RealOperator roPow;
extern RealFunction
rfSin,rfCos,rfTan,
rfAsin,rfAcos,rfAtan,
rfSinh,rfCosh,rfTanh,
rfAsinh,rfAcosh,rfAtanh,
rfExp,rfLog,
rfNeg;

extern RealFunction // cheat 
_rfCosD,_rfTanD,
_rfAsinD,_rfAcosD,_rfAtanD,
_rfTanhD,
_rfAsinhD,_rfAcoshD,_rfAtanhD,
_rfInverse,_rfSquare;

//基本函数对应表声明
extern std::map<tstring,ComplexFunction> cfMap;
extern std::map<tstring,ComplexFunction> _cfMap;
extern std::map<tstring,ComplexOperator> coMap;
extern std::map<ComplexOperator,RealOperator> roMap;
extern std::map<ComplexFunction,RealFunction> rfMap;
extern std::map<ComplexFunction,ComplexFunction> cfDerivateMap;

//函数声明
void FunctionProcessing(Postfix destFunction[/*2*/],tstring out_infix[/*2*/]);
void FunctionProcessingSE(PostfixSE destFunction[/*6*/],tstring out_infix[/*6*/]);
bool InfixStringToPostfix(const TCHAR* in,Postfix& out,bool y=false);
void RealizePostfix(const Postfix& in,PostfixSE& out);
int TestStackDepth(const Postfix& in);
int TestStackDepthSE(const PostfixSE& in);
int OpPreced(ComplexOperator op);
void ErrBox(tstring str);
int GetCpuPercentage();


#endif