/*
Favorite.cpp
收藏夹模块
*/

#include "favorite.h"
#include <algorithm>
bool FavoriteStore::Favorite::operator<(const Favorite& right)const{
	return _tcscmp(title.c_str(),right.title.c_str())<0;
}

void FavoriteStore::AddFavorite(
	const ArgPack& arg,
	const tstring& destFunction0,
	const tstring& destFunction1,
	const tstring& title){
	Favorite f;
	memcpy(&f.arg,&arg,sizeof(ArgPack));
	f.destFunction[0]=destFunction0;
	f.destFunction[1]=destFunction1;
	f.title=title;
	container.push_back(f);
	std::sort(container.begin(),container.end());
}