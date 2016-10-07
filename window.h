#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <GL/glx.h>
#include "stdafx.h"

#ifdef WIN32
#define KEY_X 'X'
#define KEY_Y 'Y'
#define KEY_Z 'Z'
#define KEY_E 'E'
#define KEY_Q 'Q'
#define KEY_U 'U'
#define KEY_I 'I'
#define KEY_O 'O'
#define KEY_A 'A'
#define KEY_S 'S'
#define KEY_D 'D'
#define KEY_F 'F'
#define KEY_G 'G'
#define KEY_H 'H'
#define KEY_J 'J'
#define KEY_K 'K'
#define KEY_B 'B'
#define KEY_N 'N'
#define KEY_M 'M'
#define KEY_C 'C'
#define KEY_V 'V'
#define KEY_Z 'Z'
#define KEY_CTRL VK_CTRL
#define KEY_SHIFT VK_SHIFT
#define KEY_ESC VK_ESCAPE
#define KEY_TAB VK_TAB
#define KEY_BACKSPACE VK_BACKSPACE
#define KEY_DELETE VK_DELETE
#define KEY_ENTER VK_RETURN
#define KEY_HOME VK_HOME
#define KEY_END VK_END
static int KEY_GROUP_ACCEL[10]={'1','2','3','4','5','6','7','8','9','0'};
#else
#define KEY_UP 111
#define KEY_LEFT 113
#define KEY_RIGHT 114
#define KEY_DOWN 116
#define KEY_C 54
#define KEY_V 55
#define KEY_X 53
#define KEY_Y 52
#define KEY_Q 24
#define KEY_E 26
#define KEY_U 30
#define KEY_I 31
#define KEY_O 32
#define KEY_A 38
#define KEY_S 39
#define KEY_D 40
#define KEY_F 41
#define KEY_G 42
#define KEY_H 43
#define KEY_J 44
#define KEY_K 45
#define KEY_B 56
#define KEY_N 57
#define KEY_M 58
#define KEY_Z 29
#define KEY_CTRL 37
#define KEY_SHIFT 50
#define KEY_ESC 9
#define KEY_TAB 23
#define KEY_BACKSPACE 22
#define KEY_DELETE 119
#define KEY_ENTER 36
#define KEY_HOME 110
#define KEY_END 115
static int KEY_GROUP_ACCEL[10]={10,11,12,13,14,15,16,17,18,19};
#endif

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
