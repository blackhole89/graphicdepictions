#ifndef _UI_H_
#define _UI_H_

#include "stdafx.h"

class CSUI {
protected:
	/* message box stuff */
	int mb_showframe;
	int mb_ytoproceed;
	int mb_modal;
	int mb_on;

	CSFont *mb_ft;
	CSTextContainer *mb_tc;

	CSTextContainer *hp_tc;
	CSTextContainer *maxhp_tc;

	CSFont *debug_ft;
	CSTextContainer *debug_tc;

    void DrawFrame(int x1,int y1,int x2,int y2);
public:
    bool showDebug;

	void dprintf(char *format,...);
	void Init();

	void DoGraphics();
	int DoLogic();

	void IncrementFrames();

	void ModalMB(int style,char *text);
};

#endif
