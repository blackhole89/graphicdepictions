#ifndef _TEXPOOL_H_
#define _TEXPOOL_H_

#include <string>
#include <map>
#include "stdafx.h"

class CSTexture;

class CSTexturePool {
public:
	std::map<std::string,CSTexture*> pool;

	CSTexture* GetTexture(char *name);
};

#endif