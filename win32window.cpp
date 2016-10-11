#include "stdafx.h"
#include "window.h"
#include <winuser.h>
#include <commdlg.h>

CSMainWindow *wnd;


#define DRAG_THRESHOLD 8

LRESULT CALLBACK WndProc(	HWND	hWnd,			
							UINT	uMsg,			
							WPARAM	wParam,			
							LPARAM	lParam)			
{
	switch (uMsg)									
	{
		case WM_ACTIVATE:							
		{
			if (!HIWORD(wParam))					
			{
				wnd->SetActive();
			}
			else
			{
				wnd->SetInactive();
			}

			return 0;								
		}
		case WM_CLOSE:								
		{
			PostQuitMessage(0);						
			wnd->Close();
			return 0;								
		}
		case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            wnd->px=LOWORD(lParam);
            wnd->py=HIWORD(lParam);
            if(ImGui::GetIO().WantCaptureMouse) {
                    //printf("click suppressed\n");
            wnd->px=wnd->py=-999;                }

            int button;
            switch(uMsg) {
            case WM_LBUTTONDOWN: button=1; break;
            case WM_MBUTTONDOWN: button=2; break;
            case WM_RBUTTONDOWN: button=3; break;
            }
            printf("button%ddown %d %d\n",button,s.e->wnd->px, s.e->wnd->py);
            s.e->keys[button]=true;
			break;
		case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            switch(uMsg) {
            case WM_LBUTTONUP: button=1; break;
            case WM_MBUTTONUP: button=2; break;
            case WM_RBUTTONUP: button=3; break;
            }

            s.e->keys[button]=false;

            if(!ImGui::GetIO().WantCaptureMouse) {
                if( (abs(wnd->px-LOWORD(lParam)) + abs(wnd->py-HIWORD(lParam)))<DRAG_THRESHOLD ) s.e->Click(button, LOWORD(lParam), HIWORD(lParam));

                s.e->CancelDragging();
            }
			break;
        case WM_MOUSEWHEEL:
            printf("wheel %08X %08X %d\n",lParam, wParam, GET_WHEEL_DELTA_WPARAM(wParam));
            if( GET_WHEEL_DELTA_WPARAM(wParam)>0 ) s.e->Click(4, LOWORD(lParam), HIWORD(lParam));
            else if( GET_WHEEL_DELTA_WPARAM(wParam)<0 ) s.e->Click(5, LOWORD(lParam), HIWORD(lParam));
            //else s.e->keys[4]=s.e->keys[5]=false;
            break;
        case WM_MOUSEMOVE:
            s.e->graphics.mx = LOWORD(lParam);
            s.e->graphics.my = HIWORD(lParam);
            break;
		case WM_KEYDOWN:
            //if(k<0xE000) ImGui::GetIO().AddInputCharacter(wParam&0xFF);

            if(!ImGui::GetIO().WantCaptureKeyboard) {
                s.e->keys[wParam&0xFF]=true;
            }
            
            ImGui::GetIO().KeysDown[wParam&0xFF]=true;
			break;
		case WM_KEYUP:
			s.e->keys[wParam&0xFF]=false;
            ImGui::GetIO().KeysDown[wParam&0xFF]=false;
			break;
        case WM_CHAR:
            if(wParam<256) ImGui::GetIO().AddInputCharacter( (ImWchar) wParam );
            break;
		case WM_PAINT:
			break;
        case WM_SIZE:
            wnd->w = LOWORD(lParam);
            wnd->h = HIWORD(lParam);
            glViewport( 0, 0, wnd->w, wnd->h );
            break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


void CSMainWindow::Create()
{
	wnd=this;

	WNDCLASS wc;

	hInstance			= GetModuleHandle(NULL);				
	wc.style			= CS_DBLCLKS | CS_OWNDC;	
	wc.lpfnWndProc		= (WNDPROC) WndProc;					
	wc.cbClsExtra		= 0;									
	wc.cbWndExtra		= 0;									
	wc.hInstance		= hInstance;							
	wc.hIcon			= NULL;		
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			
	wc.hbrBackground	= NULL;									
	wc.lpszMenuName		= NULL;									
	wc.lpszClassName	= "gdepictions";

	if (!RegisterClass(&wc))									
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	if (!(hWnd=CreateWindowEx(	0,						
								"gdepictions",			
								"graphic depictions",	
								WS_OVERLAPPEDWINDOW,	
								150, 150,				
								800,	
								600,	
								NULL,					
								NULL,					
								hInstance,				
								NULL)))					
	{
		return;
	}
	
	if (!(hDC=::GetDC(hWnd)))							// Did We Get A Device Context?
	{
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	RECT rt;
	GetClientRect(hWnd,&rt);
	SetWindowPos(hWnd,NULL,50,50,1600-(rt.right-rt.left),1200-(rt.bottom-rt.top),0);
}

void CSMainWindow::InitGL()
{
	GLuint		PixelFormat;

	static	PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,							  
		PFD_DRAW_TO_WINDOW |		  
		PFD_SUPPORT_OPENGL |		  
		PFD_DOUBLEBUFFER,			  
		PFD_TYPE_RGBA,				  
		32,							  
		0, 0, 0, 0, 0, 0,			  
		0,							  
		0,							  
		0,							  
		0, 0, 0, 0,					  
		16,							  
		8,							  
		0,							  
		PFD_MAIN_PLANE,				  
		0,							  
		0, 0, 0						  
	};
	
	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{			
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}


	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
}

void CSMainWindow::Destroy()
{
	bool fullscreen=false;
	
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("space",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

void CSMainWindow::FetchGLContext()
{
	wglMakeCurrent(hDC,wglCreateContext(hDC));
}

void CSMainWindow::Show()
{
	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
}

int CSMainWindow::CheckMessages()
{
	MSG msg;
	if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) 
	{
		if(msg.message == WM_CLOSE)
			return 0;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 1;
}

char *CSMainWindow::GetFilename(char *title, bool open)
{
    char *rbuf=(char*)malloc(512); rbuf[0]=0;

    OPENFILENAME ofn;
    ZeroMemory( &ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetHWND();
    ofn.lpstrFile = rbuf;
    ofn.nMaxFile = 512;
    ofn.lpstrFilter = "Graphs\0*.graph\0All\0*.txt\0\0";
    ofn.nFilterIndex=1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST; if(open) ofn.Flags |= OFN_FILEMUSTEXIST;

    if(open) GetOpenFileName( &ofn );
    else GetSaveFileName( &ofn );

    return rbuf;
}

void CSMainWindow::SetWindowTitle(const char *title)
{
    SetWindowText(GetHWND(),title);
}

