#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "stdafx.h"

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
#define KEY_CTRL VK_CONTROL
#define KEY_SHIFT VK_SHIFT
#define KEY_ESC VK_ESCAPE
#define KEY_TAB VK_TAB
#define KEY_BACKSPACE VK_BACKSPACE
#define KEY_DELETE VK_DELETE
#define KEY_ENTER VK_RETURN
#define KEY_HOME VK_HOME
#define KEY_END VK_END
static int KEY_GROUP_ACCEL[10] = { '1','2','3','4','5','6','7','8','9','0' };

class CSMainWindow {
protected:
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	HINSTANCE hInstance;
	bool isActive;
	bool isClosed;
public:
	int w, h;

	int px, py;

	CSMainWindow() {isActive=false;isClosed=false;}
	void Create();
	void Destroy();
	void InitGL();
	void Show();
	HWND GetHWND() { return hWnd; }
	HDC GetDC() { return hDC; }
	bool IsActive() { return isActive; }
	void SetActive() { isActive=true; }
	void SetInactive() { isActive=false; }
	bool IsClosed() { return isClosed; }
	void Close() { isClosed=true; }
	void FetchGLContext();

	int CheckMessages();

	void SetWindowTitle(const char *title);

	char *GetFilename(char *title, bool save);
};

#endif