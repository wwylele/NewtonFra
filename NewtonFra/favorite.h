#ifndef _FAVORITE_H_
#define _FAVORITE_H_

#include "main.h"


class FavoriteStore{
public:
	struct ArgPack{
		double LTx,LTy,RTx;
		double radii2;
		double escapeRadii2;
		int iterCount;
		bool showRoot;
		bool outline;
		double outlineThickness;
		double interfX,interfY;
	};
private:
	class Favorite{
	public:
		ArgPack arg;
		tstring destFunction[2];// for complex mode, destFunction[1]==_T("")
		tstring title;
		tstring imageName;
		bool operator<(const Favorite& right)const;
	};
	std::vector<Favorite> container;
public:
	void AddFavorite(
		const ArgPack& arg,
		const tstring& destFunction0,
		const tstring& destFunction1,
		const tstring& title);
	inline int GetFavoriteCount(){
		return container.size();
	}
	inline const tstring& GetFavoriteTitle(int i){
		return container[i].title;
	}
	inline void GetFavorite(int i,ArgPack& arg,tstring dest[]){
		arg=container[i].arg;
		dest[0]=container[i].destFunction[0];
		dest[1]=container[i].destFunction[1];
	}
};


#endif