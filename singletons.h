#ifndef _SINGLETONS_H_
#define _SINGLETONS_H_

#include "stdafx.h"

class CSTexturePool;
class CSMainWindow;
class CSEngine;

class CSSingletons {
public:
	CSSingletons();

	CSTexturePool *texpool;
	CSMainWindow *wnd;
	CSEngine *e;
	CSSoundSystem *s;
	CSUI *u;
};

extern CSSingletons s;

#endif
