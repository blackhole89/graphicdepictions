#include "stdafx.h"
#include "texpool.h"

CSTexture* CSTexturePool::GetTexture(char *name)
{
	if(pool.count(name)) return pool[name];

	std::string n(name);

	pool[n]=new CSTexture;

	pool[n]->name=new char[strlen(name)+1];
	strcpy(pool[n]->name,name);

	int off;

	if((off=n.rfind('/')) != std::string::npos) {
		int r,g,b;
		std::string q=n.substr(off+1,999);
		char *s=(char*)q.c_str();
		if(q.find(':') != std::string::npos) {
			int x1,y1,x2,y2;
			sscanf(s,"%d:%d:%d:%d",&x1,&y1,&x2,&y2);
			pool[n]->SubareaCopy(GetTexture((char*)n.substr(0,off).c_str()),x1,y1,x2,y2);
		} else {
			sscanf(s,"%2X%2X%2X",&r,&g,&b);
			pool[n]->CKCopy(GetTexture((char*)n.substr(0,off).c_str()),r,g,b);
		}
	} else {
		pool[n]->LoadBitmap((char*)("data/"+n).c_str());
	}

	return pool[n];
}
