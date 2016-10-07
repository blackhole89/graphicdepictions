#include "stdafx.h"
#include "singletons.h"

CSSingletons::CSSingletons()
{
	wnd=new CSMainWindow;
	e=new CSEngine;
	texpool=new CSTexturePool;
	s=new CSSoundSystem;
	u=new CSUI;
}

CSSingletons s;
