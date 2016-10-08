 /*
  * graphic depictions, a visual workbench for graphs 
  * 
  * Copyright (C) 2016 Matvey Soloviev
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS
// Windows Header Files:
#ifdef WIN32
#include <windows.h>
#define ENVSTRING "win64"
#else
#include <unistd.h>
#define ENVSTRING "linux x86_64"
#endif

#define VERSION "0.1"
#define VERSTRING VERSION" ("ENVSTRING")"

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

#include <iterator>

#define HAVE_STRUCT_TIMESPEC 1
#include "pthread.h"
#endif
//OpenGL
#include <GL/gl.h>
#include <GL/glu.h>
//application
#ifndef WIN32
#include "window.h"
#else
#include "win32window.h"
#endif
#include "texture.h"
#include "texpool.h"
#include "graphics.h"
#include "engine.h"
#include "file.h"
#include "fonts.h"
#include "export.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"

#define PI 3.14159265

#include "singletons.h"
