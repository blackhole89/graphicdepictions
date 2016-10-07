#include <GL/glxew.h>
#include "stdafx.h"
#include "window.h"

#include "imgui/imgui.h"

CSMainWindow *wnd;

#define None 0L

void CSMainWindow::Create()
{
    wnd=this;

    XInitThreads();

	XSetWindowAttributes windowAttributes;
    XEvent event;
    Colormap colorMap;
    int errorBase;
	int eventBase;

    // Open a connection to the X server
    g_pDisplay = XOpenDisplay( NULL );

    if( g_pDisplay == NULL )
    {
        fprintf(stderr, "glxsimple: %s\n", "could not open display");
        exit(1);
    }

    // Make sure OpenGL's GLX extension supported
    if( !glXQueryExtension( g_pDisplay, &errorBase, &eventBase ) )
    {
        fprintf(stderr, "glxsimple: %s\n", "X server has no OpenGL GLX extension");
        exit(1);
    }

    // Find an appropriate visual
    int doubleBufferVisual[]  =
    {
        GLX_RGBA,           // Needs to support OpenGL
        GLX_DEPTH_SIZE, 1, // Needs to support a 16 bit depth buffer
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        //GLX_ALPHA_SIZE, 1,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER,   // Needs to support double-buffering
        //GLX_SAMPLE_BUFFERS, 1,
        //GLX_SAMPLES, 4,
        None                // end of list
    };


    // Try for the double-bufferd visual first
    visualInfo = glXChooseVisual( g_pDisplay, DefaultScreen(g_pDisplay), doubleBufferVisual );

    if( visualInfo == NULL )
    {
        fprintf(stderr, "glxsimple: %s\n", "no RGB visual with depth buffer");
        exit(1);
    }

    glxContext = glXCreateContext( g_pDisplay,
                                   visualInfo,
                                   0,      // No sharing of display lists
                                   GL_TRUE ); // Direct rendering if possible

    if( glxContext == NULL )
    {
        fprintf(stderr, "glxsimple: %s\n", "could not create rendering context");
        exit(1);
    }
    /* GLXContext glxContext2 = glXCreateContext( g_pDisplay,
                                   visualInfo,
                                   glxContext, //share display lists; this is the important part
                                   GL_TRUE ); // Direct rendering if possible
    */
    // Create an X colormap since we're probably not using the default visual
    colorMap = XCreateColormap( g_pDisplay,
                                RootWindow(g_pDisplay, visualInfo->screen),
                                visualInfo->visual,
                                AllocNone );

    windowAttributes.colormap     = colorMap;
    windowAttributes.border_pixel = 0;
    windowAttributes.background_pixel = 0;
    windowAttributes.background_pixmap = None;
    windowAttributes.event_mask   = ExposureMask           |
                                    VisibilityChangeMask   |
                                    KeyPressMask           |
                                    KeyReleaseMask         |
                                    ButtonPressMask        |
                                    ButtonReleaseMask      |
                                    PointerMotionMask      |
                                    StructureNotifyMask    |
                                    SubstructureNotifyMask |
                                    FocusChangeMask;

    // Create an X window with the selected visual
    g_window = XCreateWindow( g_pDisplay,
                              RootWindow(g_pDisplay, visualInfo->screen),
                              20, 20,     // x/y position of top-left outside corner of the window
                              w=WWIDTH, h=WHEIGHT, // Width and height of window
                              0,        // Border width
                              visualInfo->depth,
                              InputOutput,
                              visualInfo->visual,
                              CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
                              &windowAttributes );

    XSetStandardProperties( g_pDisplay,
                            g_window,
                            "graphic depictions",
                            "gd",
                            None,
                            0,
                            NULL,
                            NULL );

    // Bind the rendering context to the window
    glXMakeCurrent( g_pDisplay, g_window, glxContext );

    //printf("Direct Rendering: %s\n",glXIsDirect(g_pDisplay, glxContext) ? "true" : "false");
    //printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
    //printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));

    XFree(visualInfo);

	//RECT rt;
	//GetClientRect(hWnd,&rt);
	//SetWindowPos(hWnd,NULL,50,50,1600-(rt.right-rt.left),1200-(rt.bottom-rt.top),0);
}

void CSMainWindow::InitGL()
{

}

void CSMainWindow::Destroy()
{

}

void CSMainWindow::Show()
{
    // Request the X window to be displayed on the screen
    XMapWindow( g_pDisplay, g_window );
}

#define DRAG_THRESHOLD 8

int CSMainWindow::CheckMessages()
{
    XEvent event;
	while (XPending(g_pDisplay) ) // Loop to compress events
    {
        XNextEvent( g_pDisplay, &event );

        switch( event.type )
        {
            case ButtonPress:
            {
                px=event.xbutton.x;
                py=event.xbutton.y;
                if(ImGui::GetIO().WantCaptureMouse) {
                    //printf("click suppressed\n");
                    px=py=-999;                }
                s.e->keys[event.xbutton.button]=true;
            }
            break;

            case ButtonRelease:
            {
                //printf("buttonrelease %d (%d,%d) from (%d,%d)\n",event.xbutton.button,event.xbutton.x,event.xbutton.y,px,py);
                if( (abs(px-event.xbutton.x) + abs(py-event.xbutton.y))<DRAG_THRESHOLD ) s.e->Click(event.xbutton.button,event.xbutton.x,event.xbutton.y);
                s.e->keys[event.xbutton.button]=false;
                s.e->CancelDragging();
            }
            break;

            case KeyPress:
            {
//                if(!s.e->keys[event.xkey.keycode&0xFF]) s.ed->HandleKey(event.xkey.keycode&0xFF);
                char buf[16]; KeySym k;
                XLookupString(&event.xkey,buf,16,&k,NULL);


                if(k<0xE000) ImGui::GetIO().AddInputCharacter(k);


                if(!ImGui::GetIO().WantCaptureKeyboard) {
                    s.e->keys[event.xkey.keycode&0xFF]=true;
                }

                ImGui::GetIO().KeysDown[event.xkey.keycode&0xFF]=true;
                //fprintf( stderr, "KeyPress event\n" );
            }
            break;

            case KeyRelease:
            {
                s.e->keys[event.xkey.keycode&0xFF]=false;
                ImGui::GetIO().KeysDown[event.xkey.keycode&0xFF]=false;
                //fprintf( stderr, "KeyRelease event\n" );
            }
            break;

            case MotionNotify:
            {
                s.e->graphics.mx=event.xmotion.x;
                s.e->graphics.my=event.xmotion.y;

                /* TODO: Should this really be here? */
                /*if( s.e->keys[1] && px!=-999 && py!=-999 && (abs(px-event.xmotion.x) + abs(py-event.xmotion.y))>=DRAG_THRESHOLD ) {
                    s.e->StartDragging(px,py);
                }*/
            }
            break;

            case Expose:
            {
                SetActive();
                fprintf( stderr, "Expose event\n" );
            }
            break;

            case ConfigureNotify:
            {
                glViewport( 0, 0, w=event.xconfigure.width, h=event.xconfigure.height );
            }
            break;
        }
    }

	return 1;
}

void CSMainWindow::SetWindowTitle(const char *title)
{
    #ifdef WIN32
    SetWindowText(GetHWND(),text);
    #else
    XSetStandardProperties( g_pDisplay,
                        g_window,
                        title,
                        "gd",
                        None,
                        0,
                        NULL,
                        NULL );
    #endif
}

char *CSMainWindow::GetFilename(char *title, bool save)
{
    static char buf[128];
    sprintf(buf,"zenity --file-selection --title=%s %s",title,save?"--save":"");
    FILE *ofd = popen(buf,"r");
    char *fn = NULL;
    fscanf(ofd," %m[^\n] ",&fn);
    fclose(ofd);
    return fn;}
