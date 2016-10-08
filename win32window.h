#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "stdafx.h"

class CSMainWindow {
protected:
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	HINSTANCE hInstance;
	bool isActive;
	bool isClosed;
public:
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
};

#endif