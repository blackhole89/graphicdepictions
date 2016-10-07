#include "stdafx.h"
#include "ui.h"

void CSUI::Init()
{
	mb_modal=0;
	mb_ytoproceed=0;
	mb_showframe=0;
	mb_on=0;

	mb_ft=new CSFont;
	mb_ft->LoadFont("georgia.ttf",19,2);

	mb_tc=new CSTextContainer;
	mb_tc->Initialize(1024,256,1024,2,mb_ft,true);

    hp_tc=new CSTextContainer;
    hp_tc->Initialize(128,32,10,1,mb_ft);

    maxhp_tc=new CSTextContainer;
    maxhp_tc->Initialize(128,32,10,1,mb_ft);

	debug_ft=new CSFont;
	debug_ft->LoadFont("StrLit.ttf",9,0);

	debug_tc=new CSTextContainer;
	debug_tc->Initialize(640,320,32768,0,debug_ft,false);

	dprintf("Onscreen debug has started. Time is %d.\n",time(NULL));
}

void CSUI::dprintf(char *format,...)
{
	char buf[4096];
	va_list argptr;
	va_start(argptr, format);
	vsprintf(buf,format,argptr);
	va_end(argptr);
	debug_tc->AppendText(buf);
}

void CSUI::DrawFrame(int x1,int y1,int x2,int y2)
{
    glDisable(GL_TEXTURE_2D);
	glColor4f(0.02,0.04,0.2,0.95);
	glBegin(GL_QUADS);
		glVertex2f(x1,y1);
		glVertex2f(x2,y1);
		glColor4f(0.01,0.02,0.1,0.95);
		glVertex2f(x2,y2);
		glVertex2f(x1,y2);
	glEnd();
	glLineWidth(3);
	glColor3f(1,1,1);
	glEnable(GL_LINE_WIDTH);
	glBegin(GL_LINE_STRIP);
		glVertex2f(x1+1,y1+1);
		glVertex2f(x2-1,y1+1);
		glVertex2f(x2-1,y2-1);
		glVertex2f(x1+1,y2-1);
		glVertex2f(x1+1,y1+1);
	glEnd();
	glDisable(GL_LINE_WIDTH);
}

void CSUI::DoGraphics()
{
	/* draw HP/MAXHP */
	glPushMatrix();

	glTranslatef(10,10,0);
	hp_tc->Render();
	glTranslatef(60,0,0);
	maxhp_tc->Render();
	//glTranslatef(400,0,0);
	if(showDebug) debug_tc->Render();

	glPopMatrix();

	glPushMatrix();

	glTranslatef(0,WHEIGHT-120,0);

	if(mb_showframe) {
        DrawFrame(0,0,WWIDTH,120);
	}
	glTranslatef(10,10,0);
	if(mb_on) {
		mb_tc->Render();
		if(mb_tc->IsWaiting()) {
			glDisable(GL_TEXTURE_2D);

			glTranslatef(WWIDTH-30,95,0);
			glBegin(GL_TRIANGLES);
				glVertex2f(0,0);
				glVertex2f(10,0);
				glVertex2f(5,5);
			glEnd();
		}
	}

	glPopMatrix();
}

/* LOGIC */

#ifndef WIN32
#define KEYCODE 52
#else
#define KEYCODE 'Y'
#endif

void CSUI::IncrementFrames()
{
   	mb_tc->IncrementFrame();


    char buf[16];
/*	sprintf(buf,"/%d",s.e->p->b.maxhp);
	maxhp_tc->MaybeReplaceText(buf); maxhp_tc->IncrementFrame();
	sprintf(buf,"%d",s.e->p->b.hp);
	hp_tc->MaybeReplaceText(buf); hp_tc->IncrementFrame(); */
	debug_tc->IncrementFrame();
}

int CSUI::DoLogic()
{
    IncrementFrames();
	if(mb_tc->IsDone()) { mb_on=0; mb_showframe=0; mb_ytoproceed=0; }
	if(mb_ytoproceed) {
		if(s.e->keys[KEYCODE]) {
			mb_tc->SendConfirm();
		}
		s.e->keys[KEYCODE]=0; // eat the Y
		if(mb_modal && !mb_tc->IsDone()) return 1; //no gameplay proceeding
	}
	return 0;
}

void CSUI::ModalMB(int style,char *text)
{
	mb_showframe=style;
	mb_modal=1;
	mb_on=1;
	mb_ytoproceed=1;
	mb_tc->ReplaceText(text);
}
