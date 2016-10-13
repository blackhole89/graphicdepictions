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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <GL/glx.h>
#include "stdafx.h"

#define KEY_LEFT (XK_Left&0xff)
#define KEY_UP (XK_Up&0xff)
#define KEY_RIGHT (XK_Right&0xff)
#define KEY_DOWN (XK_Down&0xff)

#define KEY_SHIFT (XK_Shift_L&0xff)
#define KEY_CTRL (XK_Control_L&0xff)
#define KEY_DELETE (XK_Delete&0xff)
#define KEY_ENTER (XK_Return&0xff)
#define KEY_BACKSPACE (XK_BackSpace&0xff)
#define KEY_HOME (XK_Home&0xff)
#define KEY_END (XK_End&0xff)
#define KEY_ESC (XK_Escape&0xff)
#define KEY_TAB (XK_Tab&0xff)

#define KEY_X 'x'
#define KEY_Y 'y'
#define KEY_Z 'z'
#define KEY_E 'e'
#define KEY_Q 'q'
#define KEY_U 'u'
#define KEY_I 'i'
#define KEY_O 'o'
#define KEY_A 'a'
#define KEY_S 's'
#define KEY_D 'd'
#define KEY_F 'f'
#define KEY_G 'g'
#define KEY_H 'h'
#define KEY_J 'j'
#define KEY_K 'k'
#define KEY_B 'b'
#define KEY_N 'n'
#define KEY_M 'm'
#define KEY_C 'c'
#define KEY_V 'v'
#define KEY_Z 'z'

static int KEY_GROUP_ACCEL[10]={'1','2','3','4','5','6','7','8','9','0'};

#define WHEIGHT 480
#define WWIDTH 640

class CSMainWindow {
protected:
	bool isActive;
	bool isClosed;
public:
    int w,h;

    int px,py;  //button pressing coords

    Display *g_pDisplay;
    Window   g_window;
    XVisualInfo *visualInfo;
    GLXContext glxContext,glxContext2;

	CSMainWindow() {isActive=false;isClosed=false;}
	void Create();
	void Destroy();
	void InitGL();
	void Show();
	bool IsActive() { return isActive; }
	void SetActive() { isActive=true; }
	void SetInactive() { isActive=false; }
	bool IsClosed() { return isClosed; }
	void Close() { isClosed=true; }

	void SetWindowTitle(const char *title);

	char *GetFilename(char *title, bool save);

	int CheckMessages();
};

#endif
