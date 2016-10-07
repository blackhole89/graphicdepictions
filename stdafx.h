// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// no idea where these even come from
#undef None
#undef True
#undef False
#include "v8.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <deque>
#include <algorithm>
#ifndef WIN32
#include <pthread.h>
int pthread_delay_np(timespec *ts);
#else
#include "pthread.h"
#endif
//OpenGL
#include <GL/gl.h>
#include <GL/glu.h>
//application
#include "window.h"
#include "texture.h"
#include "texpool.h"
#include "graphics.h"
#include "engine.h"
#include "file.h"
#include "sound.h"
#include "fonts.h"
#include "ui.h"
#include "export.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"

#define PI 3.14159265

#include "singletons.h"
