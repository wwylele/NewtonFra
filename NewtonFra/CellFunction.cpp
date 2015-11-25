/*
CellFunction.cpp
基本函数的定义
*/

#include "main.h"

//复数运算符
ComplexOperator
coAdd(std::operator+),
coSub(std::operator-),
coMul(std::operator*),
coDiv(std::operator/),
coPow(std::pow);

//实数运算符
double roAdd(double a,double b){
	return a+b; 
}
double roSub(double a,double b){
	return a-b;
}
double roMul(double a,double b){
	return a*b;
}
double roDiv(double a,double b){
	return a/b;
}
RealOperator roPow(pow);

//复数函数
ComplexFunction
cfSin(std::sin),
cfCos(std::cos),
cfTan(std::tan),
cfAsin(std::asin),
cfAcos(std::acos),
cfAtan(std::atan),
cfSinh(std::sinh),
cfCosh(std::cosh),
cfTanh(std::tanh),
cfAsinh(std::asinh),
cfAcosh(std::acosh),
cfAtanh(std::atanh),
cfExp(std::exp),
cfLog(std::log),
cfNeg(std::operator-);

//实数函数
RealFunction
rfSin(sin),
rfCos(cos),
rfTan(tan),
rfAsin(asin),
rfAcos(acos),
rfAtan(atan),
rfSinh(sinh),
rfCosh(cosh),
rfTanh(tanh),
rfAsinh(asinh),
rfAcosh(acosh),
rfAtanh(atanh),
rfExp(exp),
rfLog(log),
rfNeg([](double a)->double{return -a;});

//内部复数函数
ComplexFunction
_cfCosD([](const Complex&a)->Complex{
	return -std::sin(a);
}),
_cfTanD([](const Complex&a)->Complex{
	Complex c=std::cos(a);
	return 1.0/(c*c);
}),
_cfAsinD([](const Complex&a)->Complex{
	return 1.0/std::sqrt(1.0-a*a);
}),
_cfAcosD([](const Complex&a)->Complex{
	return -1.0/std::sqrt(1.0-a*a);
}),
_cfAtanD([](const Complex&a)->Complex{
	return 1.0/(1.0+a*a);
}),
_cfTanhD([](const Complex&a)->Complex{
	Complex c=std::cosh(a);
	return 1.0/(c*c);
}),
_cfAsinhD([](const Complex&a)->Complex{
	return 1.0/std::sqrt(1.0+a*a);
}),
_cfAcoshD([](const Complex&a)->Complex{
	return 1.0/std::sqrt(a*a-1.0);
}),
_cfAtanhD([](const Complex&a)->Complex{
	return 1.0/(1.0-a*a);
}),
_cfInverse([](const Complex&a)->Complex{
	return 1.0/a;
}),
_cfSquare([](const Complex& c)->Complex{
	return c*c;
});

//实数内部函数
RealFunction
_rfCosD([](double a)->double{
	return -sin(a);
}),
_rfTanD([](double a)->double{
	double c=cos(a);
	return 1.0/(c*c);
}),
_rfAsinD([](double a)->double{
	return 1.0/sqrt(1.0-a*a);
}),
_rfAcosD([](double a)->double{
	return -1.0/sqrt(1.0-a*a);
}),
_rfAtanD([](double a)->double{
	return 1.0/(1.0+a*a);
}),
_rfTanhD([](double a)->double{
	double c=cosh(a);
	return 1.0/(c*c);
}),
_rfAsinhD([](double a)->double{
	return 1.0/sqrt(1.0+a*a);
}),
_rfAcoshD([](double a)->double{
	return 1.0/sqrt(a*a-1.0);
}),
_rfAtanhD([](double a)->double{
	return 1.0/(1.0-a*a);
}),
_rfInverse([](double a)->double{
	return 1.0/a;
}),
_rfSquare([](double a)->double{
	return a*a;
});

//实数复数运算符对应表
#define PAIR_ROMAP(O) std::pair<ComplexOperator,RealOperator>{co##O,ro##O}
std::map<ComplexOperator,RealOperator> roMap={
	PAIR_ROMAP(Add),
	PAIR_ROMAP(Sub),
	PAIR_ROMAP(Mul),
	PAIR_ROMAP(Div),
	PAIR_ROMAP(Pow),
};

//实数复数函数对应表
#define PAIR_RFMAP(F) std::pair<ComplexFunction,RealFunction>{cf##F,rf##F}
#define _PAIR_RFMAP(F) std::pair<ComplexFunction,RealFunction>{_cf##F,_rf##F}
std::map<ComplexFunction,RealFunction> rfMap={
	PAIR_RFMAP(Sin),
	PAIR_RFMAP(Cos),
	PAIR_RFMAP(Tan),
	PAIR_RFMAP(Sinh),
	PAIR_RFMAP(Cosh),
	PAIR_RFMAP(Tanh),
	PAIR_RFMAP(Asin),
	PAIR_RFMAP(Acos),
	PAIR_RFMAP(Atan),
	PAIR_RFMAP(Asinh),
	PAIR_RFMAP(Acosh),
	PAIR_RFMAP(Atanh),
	PAIR_RFMAP(Exp),
	PAIR_RFMAP(Log),
	PAIR_RFMAP(Neg),
	_PAIR_RFMAP(CosD),
	_PAIR_RFMAP(TanD),
	_PAIR_RFMAP(TanhD),
	_PAIR_RFMAP(AsinD),
	_PAIR_RFMAP(AcosD),
	_PAIR_RFMAP(AtanD),
	_PAIR_RFMAP(AsinhD),
	_PAIR_RFMAP(AcoshD),
	_PAIR_RFMAP(AtanhD),
	_PAIR_RFMAP(Inverse),
	_PAIR_RFMAP(Square),
};

//复数函数-字符串对应表
std::map<tstring,ComplexFunction> cfMap={
	std::pair<tstring,ComplexFunction>(_T("sin"),cfSin),
	std::pair<tstring,ComplexFunction>(_T("cos"),cfCos),
	std::pair<tstring,ComplexFunction>(_T("tan"),cfTan),
	std::pair<tstring,ComplexFunction>(_T("asin"),cfAsin),
	std::pair<tstring,ComplexFunction>(_T("acos"),cfAcos),
	std::pair<tstring,ComplexFunction>(_T("atan"),cfAtan),
	std::pair<tstring,ComplexFunction>(_T("sinh"),cfSinh),
	std::pair<tstring,ComplexFunction>(_T("cosh"),cfCosh),
	std::pair<tstring,ComplexFunction>(_T("tanh"),cfTanh),
	std::pair<tstring,ComplexFunction>(_T("asinh"),cfAsinh),
	std::pair<tstring,ComplexFunction>(_T("acosh"),cfAcosh),
	std::pair<tstring,ComplexFunction>(_T("atanh"),cfAtanh),
	std::pair<tstring,ComplexFunction>(_T("exp"),cfExp),
	std::pair<tstring,ComplexFunction>(_T("log"),cfLog),
	std::pair<tstring,ComplexFunction>(_T("neg"),cfNeg),
};
std::map<tstring,ComplexFunction> _cfMap={
	std::pair<tstring,ComplexFunction>(_T("cos`"),_cfCosD),
	std::pair<tstring,ComplexFunction>(_T("tan`"),_cfTanD),
	std::pair<tstring,ComplexFunction>(_T("asin`"),_cfAsinD),
	std::pair<tstring,ComplexFunction>(_T("acos`"),_cfAcosD),
	std::pair<tstring,ComplexFunction>(_T("atan`"),_cfAtanD),
	std::pair<tstring,ComplexFunction>(_T("tanh`"),_cfTanhD),
	std::pair<tstring,ComplexFunction>(_T("asinh`"),_cfAsinhD),
	std::pair<tstring,ComplexFunction>(_T("acosh`"),_cfAcoshD),
	std::pair<tstring,ComplexFunction>(_T("atanh`"),_cfAtanhD),
	std::pair<tstring,ComplexFunction>(_T("inverse"),_cfInverse),
	std::pair<tstring,ComplexFunction>(_T("square"),_cfSquare),
};
//复数运算符-字符串对应表
std::map<tstring,ComplexOperator> coMap={
	std::pair<tstring,ComplexOperator>(_T("+"),coAdd),
	std::pair<tstring,ComplexOperator>(_T("-"),coSub),
	std::pair<tstring,ComplexOperator>(_T("*"),coMul),
	std::pair<tstring,ComplexOperator>(_T("/"),coDiv),
	std::pair<tstring,ComplexOperator>(_T("^"),coPow),
};

//复数函数-导数对应表
std::map<ComplexFunction,ComplexFunction> cfDerivateMap={
	std::pair<ComplexFunction,ComplexFunction>{cfSin,cfCos},
	std::pair<ComplexFunction,ComplexFunction>{cfCos,_cfCosD},
	std::pair<ComplexFunction,ComplexFunction>{cfTan,_cfTanD},
	std::pair<ComplexFunction,ComplexFunction>{cfAsin,_cfAsinD},
	std::pair<ComplexFunction,ComplexFunction>{cfAcos,_cfAcosD},
	std::pair<ComplexFunction,ComplexFunction>{cfAtan,_cfAtanD},
	std::pair<ComplexFunction,ComplexFunction>{cfSinh,cfCosh},
	std::pair<ComplexFunction,ComplexFunction>{cfCosh,cfSinh},
	std::pair<ComplexFunction,ComplexFunction>{cfTanh,_cfTanhD},
	std::pair<ComplexFunction,ComplexFunction>{cfAsinh,_cfAsinhD},
	std::pair<ComplexFunction,ComplexFunction>{cfAcosh,_cfAcoshD},
	std::pair<ComplexFunction,ComplexFunction>{cfAtanh,_cfAtanhD},
	std::pair<ComplexFunction,ComplexFunction>{cfExp,cfExp},
	std::pair<ComplexFunction,ComplexFunction>{cfLog,_cfInverse},
};