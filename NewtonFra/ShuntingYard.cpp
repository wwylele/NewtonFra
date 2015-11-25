/*
ShuntingYard.cpp
字符串表达式转后缀表达式
*/

#include "main.h"

#include <stack>


enum CHAR_NATURE{
	UNKNOWN,
	NUMBER,
	LETTER,//include '_','`'
	MARK
};
CHAR_NATURE CharNature(TCHAR in){
	if(in>=48 && in<=57)return NUMBER;
	if(in==46)return NUMBER;
	if(in>=65 && in<=90)return LETTER;
	if(in>=95 && in<= 122)return LETTER;
	if(in==_T('+') || in==_T('-') || in==_T('*') || in==_T('/') || in==_T('^')||
		in==_T('(') || in==_T(')'))return MARK;

	return UNKNOWN;
}
int OpPreced(ComplexOperator op)
{
	if(op==coPow)return 3;
	if(op==coMul||op==coDiv)return 2;
	if(op==coAdd||op==coSub)return 1;
	return 0;
}


bool InfixStringToPostfix(const TCHAR* in,Postfix& out,bool y){
	typedef std::list<Token> Infix;

	out.clear();

	tstring input(in);

	//删除输入的所有空格
	while(1){
		size_t begin=0;
		begin=input.find(_T(" "),begin);
		if(begin==tstring::npos)break;
		input.replace(begin,1,_T(""));
	}

	Infix infix;
	//将字符串初步转换成符号列
	for(auto iter=input.begin();iter!=input.end();){
		CHAR_NATURE nature;
		nature=CharNature(*iter);
		if(nature==UNKNOWN){
			ErrBox(tstring(_T("无法解读的字符:"))+*iter);
			return false;
		}
		else if(nature==MARK){
			try{
				infix.emplace_back(coMap.at(tstring(1,*iter)));
			}
			catch(std::out_of_range){
				infix.emplace_back((char)*iter);
			}
			iter++;
		}
		else if(nature==NUMBER){
			auto jter=iter;
			while(iter!=input.end() && CharNature(*iter)==NUMBER)iter++;
			infix.emplace_back(Complex(_ttof(tstring(jter,iter).c_str()),0.0));
		}
		else if(nature==LETTER){
			auto jter=iter;
			while(iter!=input.end() &&
				(CharNature(*iter)==NUMBER||CharNature(*iter)==LETTER))iter++;
			tstring letter(jter,iter);
			if(letter==_T("x")){
				infix.emplace_back();
			}
			else if(letter==_T("y")){
				if(!y){
					ErrBox(_T("复变函数模式不允许使用y"));
					return false;
				}
				infix.emplace_back(true);
			}
			else if(letter==_T("i")){
				infix.emplace_back(Complex(0.0,1.0));
			}
			else{
				try{
					infix.emplace_back(cfMap.at(letter));
				}
				catch(std::out_of_range){
					ErrBox(tstring(_T("未定义函数:"))+letter);
					return false;
				}
				
			}
		}
	}

	//进一步处理符号列:减号转负号标记
	for(auto iter=infix.begin();iter!=infix.end();iter++){

		if(iter->type=='o' && iter->pOperator==coSub){
			if(iter!=infix.begin()){
				auto jter=iter;
				iter--;
				if(iter->type!='c' &&
					iter->type!='x' &&
					iter->type!='y' &&
					iter->type!=')'){
					jter->type='_';
				}
				iter=jter;
			}
			else{
				iter->type='_';
			}
		}
	}

	//中缀转后缀
	std::stack<Token> operatorstack;
	for(auto iter=infix.begin();iter!=infix.end();iter++){
		if(iter->type=='c' || iter->type=='x' || iter->type=='y'){
			out.push_back(*iter);
		}
		else if(iter->type=='f'){
			operatorstack.push(*iter);
		}
		/*else if(iter->name==","){
			while(1){
				if(operatorstack.empty()){
					throw exception("[ShuntingYard]括号不匹配");
				}
				if(operatorstack.top().name=="(")break;
				out.push_back(operatorstack.top());
				operatorstack.pop();
			}
		}*/
		else if(iter->type=='('){
			operatorstack.push(*iter);
		}
		else if(iter->type==')'){
			while(1){
				if(operatorstack.empty()){
					ErrBox(_T("括号不匹配"));
					return false;
				}
				if(operatorstack.top().type=='(')break;
				out.push_back(operatorstack.top());
				operatorstack.pop();
			}
			operatorstack.pop();
			if((!operatorstack.empty()) &&
				operatorstack.top().type=='f'){
				out.push_back(operatorstack.top());
				operatorstack.pop();
			}
		}
		else if(iter->type=='o' || iter->type=='_'){// +-*/^
			int preced;
			if((preced=OpPreced(iter->pOperator))==0){
				ErrBox(_T("无法识别"));
				return false;
			}
			while(!operatorstack.empty() && 
				(operatorstack.top().type=='o'|| operatorstack.top().type=='_')&&
				OpPreced(operatorstack.top().pOperator)>=preced){
				out.push_back(operatorstack.top());
				operatorstack.pop();
			}
			operatorstack.push(*iter);
		}
		else {
			ErrBox(_T("无法识别"));
			return false;
		}
	}
	while(!operatorstack.empty()){
		if(operatorstack.top().type=='(' || operatorstack.top().type==')'){
			ErrBox(_T("括号不匹配"));
			return false;
		}
		out.push_back(operatorstack.top());
		operatorstack.pop();
	}

	//负号最终处理 & 后缀检查
	int depth=0;
	for(auto iter=out.begin();iter!=out.end();iter++){
		switch(iter->type){
		case 'c':case 'x':case 'y':depth++;break;
		case '_':
			*iter=Token(cfNeg);
		case 'f':
			if(depth==0){
				ErrBox(_T("参数不匹配"));
				return false;
			}
			break;
		case 'o':
			if(depth<2){
				ErrBox(_T("参数不匹配"));
				return false;
			}
			depth--;
			break;
		
		}
	}
	if(depth!=1){
		ErrBox(_T("参数不匹配"));
		return false;
	}
	return true;
}