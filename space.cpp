// space.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "space.h"
#define MAX_LOADSTRING 100

// Global Variables:
//HINSTANCE hInst;								// current instance


#ifdef WIN32
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
	srand((unsigned)time(NULL));

	s.wnd->Create();
	s.wnd->InitGL();
	s.wnd->Show();

	s.e->Init(s.wnd);
	s.e->Run();

	s.wnd->Destroy();

	return 0;
}

