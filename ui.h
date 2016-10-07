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
