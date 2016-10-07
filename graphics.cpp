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

#include "stdafx.h"
#include "graphics.h"

#include <math.h>

void CSGraphics::Init(CSEngine *ep)
{
	e=ep;

	tx=ty=tz=0.0f;
	angle=vangle=0.0f;
	zoom=1.0f;

	framectr=0;

	dragging1=dragging2=dragging3=false;

	strcpy(nodelook[5],"label");
	for(int i=0;i<9;++i) for(int j=0;j<4;++j) nodecol[i][j]=1.0f;

	/* default colors */
	#define A4(x,a,b,c,d) x[0]=a; x[1]=b; x[2]=c; x[3]=d;
	A4(config.bg, 1.0, 1.0, 1.0, 1.0);
	A4(config.grid, 0.7, 0.7, 0.7, 1.0);
	A4(config.edge, 0.4, 0.4, 0.8, 0.6);
	A4(config.n_edge, 0.0, 0.0, 0.0, 1.0);
	A4(config.n_select, 1.0, 0.0, 0.0, 1.0);
	A4(config.n_fill, 1.0, 1.0, 1.0, 1.0);
	A4(config.selection, 1.0, 0.0, 0.0, 0.5);
	#undef A4

	nodefont=new CSFont;
	nodefont->LoadFont("StrLit.ttf",9,0);

	ImGui_ImplGlfw_Init(s.e->wnd, true);

	ImGui::GetIO().Fonts->AddFontFromFileTTF("StrLit.ttf", 16.0f);
    ImGui::GetStyle().WindowRounding=1.0f;
    ImGui::GetStyle().FramePadding=ImVec2(5,2);
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4( 0.1f,0.1f,0.3f,0.8f);
    ImGui::GetStyle().Colors[ImGuiCol_Border] = ImVec4(0.9,0.9,1,1.0);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4( 0.4f,0.4f,0.6f,0.8f);
    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.4f,0.4f,0.6f,0.8f);
    ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4( 0.05f,0.05f,0.15f,0.8f);

    ImGui_ImplGlfw_NewFrame();
}

void CSGraphics::OrthoAtSpatial(float x, float y, float z)
{
    GLint realy;
    GLdouble win_x,win_y,win_z;
    int viewport[4];
    double mvmatrix[16],projmatrix[16];

    glGetIntegerv(GL_VIEWPORT,viewport);

    glGetDoublev(GL_MODELVIEW_MATRIX,mvmatrix);
    glGetDoublev(GL_PROJECTION_MATRIX,projmatrix);

    gluProject((GLdouble)x,(GLdouble)y,(GLdouble)z,mvmatrix,projmatrix,viewport,&win_x,&win_y,&win_z);

    realy = viewport[3]-(GLint)win_y -1;// 左上角为坐标原点

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,s.wnd->w,s.wnd->h,0,0,1000);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(win_x,realy,0);

    glPushAttrib(GL_ENABLE_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

void CSGraphics::ReturnToSpace()
{
    glPopAttrib();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

void CSGraphics::ResetView()
{
    tx=ty=tz=0;
    angle=vangle=0;}

void CSGraphics::SpaceToScreen(float x,float y,float z, float& sxout, float& syout)
{
    GLdouble sx,sy,sz;

    gluProject((GLdouble)x,(GLdouble)y,(GLdouble)z,c_mvmatrix,c_projmatrix,c_viewport,&sx,&sy,&sz);

    sxout = sx;
    syout = c_viewport[3]-sy -1;// 左上角为坐标原点
}

void CSGraphics::ScreenToSpace(float x,float y,float& sxout, float& syout, float &szout)
{
    GLdouble sx,sy,sz;

    gluUnProject((GLdouble)x,(GLdouble)c_viewport[3]-y,(GLdouble)0.5,c_mvmatrix,c_projmatrix,c_viewport,&sx,&sy,&sz);

    sxout = sx;
    syout = sy;
    szout = sz;
}

void CSGraphics::SetupCamera()
{
    float aspect = s.wnd->h/(float)s.wnd->w;
	glOrtho(-0.5*zoom, 0.5*zoom,0.5*zoom*aspect,-0.5*zoom*aspect,-1.0f, 3.0f);

    /* RO-style camera smoothing */
    static float ttx=tx,tty=ty,ttz=tz;

    ttx=(0.25*tx+ttx)/1.25f;
    tty=(0.25*ty+tty)/1.25f;
    ttz=(0.25*tz+ttz)/1.25f;

    gluLookAt(ttx-sin(angle)*cos(vangle)*1.0f,tty-sin(vangle)*1.0f,ttz+cos(angle)*cos(vangle)*1.0f,
      ttx,tty,ttz, 0,1,0);

    glGetIntegerv(GL_VIEWPORT,c_viewport);

    glGetDoublev(GL_MODELVIEW_MATRIX,c_mvmatrix);
    glGetDoublev(GL_PROJECTION_MATRIX,c_projmatrix);

}

void CSGraphics::DoDraw()
{
    ++framectr;

    glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,s.wnd->w,s.wnd->h,0,0,1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor4fv(config.bg);
	glBegin(GL_QUADS);
        glVertex2f(0,0);
        glVertex2f(s.wnd->w,0);
        glVertex2f(s.wnd->w,s.wnd->h);
        glVertex2f(0,s.wnd->h);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	SetupCamera();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor4fv(config.grid);
	glBegin(GL_LINES);
        glVertex3f(-10,0,0);
        glVertex3f(10,0,0);
        glVertex3f(0,-10,0);
        glVertex3f(0,10,0);
        glVertex3f(0,0,-10);
        glVertex3f(0,0,10);
	glEnd();

	glPointSize(3);
	glBegin(GL_POINTS);
        for(int y=-10;y<=10;++y)
            for(int x=-10;x<=10;++x)
            {
                glVertex3f(x*0.1f,y*0.1f,0.0);
            }
	glEnd();

	//e->p->Render(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	//glTranslatef((s.wnd->aspect/2.0)-0.5,0,0);

	glDisable(GL_DEPTH_TEST);
    glLineWidth(3);
    for(auto i = s.e->st.edges.begin(); i!=s.e->st.edges.end(); ++i) {
        glColor4fv(config.edge);
        glBegin(GL_LINES);
            glVertex3fv( (*i)->n1->pos );
            glVertex3fv( (*i)->n2->pos );
        glEnd();
	}
	glLineWidth(1);
	glEnable(GL_DEPTH_TEST);

	for(auto i = s.e->st.nodes.begin(); i!=s.e->st.nodes.end(); ++i) {
        OrthoAtSpatial( (*i)->pos[0], (*i)->pos[1], (*i)->pos[2] );


        if( (*i)->selected ) {
            glColor4fv(config.n_select);

            glBegin(GL_QUADS);
                glVertex2f(-8,-8);
                glVertex2f(+7,-8);
                glVertex2f(+7,+7);
                glVertex2f(-8,+7);
            glEnd();
            glLineWidth(1);
        }


        glColor4fv(config.n_fill);
        glBegin(GL_QUADS);
            glVertex2f(-5,-5);
            glVertex2f(+5,-5);
            glVertex2f(+5,+5);
            glVertex2f(-5,+5);
        glEnd();

        glColor4fv(config.n_edge);
        glBegin(GL_LINE_LOOP);
            glVertex2f(-5,-5);
            glVertex2f(+5,-5);
            glVertex2f(+5,+5);
            glVertex2f(-5,+5);
        glEnd();

        /* labels */
        for(int y=0;y<3;++y) for(int x=0;x<3;++x) {
            if( (*i)->a.count( nodelook[y*3+x] ) ) {
                glColor4fv( nodecol[y*3+x] );
                CSState::CSAttr *a = &(*i)->a[nodelook[y*3+x]];
                char buf[32]={0};
                switch(a->type) {
                case CSState::CSAttr::TY_INT:
                    sprintf(buf,"%d",a->data.d_int);
                    break;
                case CSState::CSAttr::TY_BITS:
                    int b;
                    for(b=0;b<=floor(0.01f+log(a->data.d_bits)/log(2.0f));++b) {
                        buf[b]=(a->data.d_bits&(1<<b))?'1':'0';
                    }
                    buf[b]=0;
                    if(!b) {
                        buf[b]='0';
                        buf[b+1]=0;
                    }
                    break;
                case CSState::CSAttr::TY_FLOAT:
                    sprintf(buf,"%.3f",a->data.d_float);
                    break;
                }

                glTranslatef((x*15)-23,(y*13)-19,0);
                int n=strlen(buf);
                if(!x) glTranslatef(-7*n+15,0,0);
                nodefont->Render(16,buf,n);
                if(!x) glTranslatef(7*n-15,0,0);
                glTranslatef(23-(x*15),19-(y*13),0);
            }
        }

        ReturnToSpace();    }


    /* draw selection frame */
    if(dragging1 && s.e->action==CSEngine::AC_SELECT) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0,s.wnd->w,s.wnd->h,0,0,1000);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4fv(config.selection);
        glBegin(GL_LINE_LOOP);
            glVertex2f(sx0, sy0);
            glVertex2f(sx0, sy1);
            glVertex2f(sx1, sy1);
            glVertex2f(sx1, sy0);
        glEnd();

        glDisable(GL_BLEND);
    }


	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);

    ImGui::Render();

    ImGui_ImplGlfw_NewFrame();

	#ifdef WIN32
	SwapBuffers(e->wnd->GetDC());
	#else
	glXSwapBuffers(e->wnd->g_pDisplay,e->wnd->g_window );
	//glFinish();
	#endif
}

void CSGraphics::IncrementFrame()
{
}
