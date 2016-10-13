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

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "stdafx.h"
#include "imgui/imgui.h"
#include <vector>

class CSEngine;
class CSEffect;
class CSFont;

class CSGraphics {
public:
	CSEngine *e;

	int framectr;

    float mx, my;

    struct {
        float bg[4];
        float grid[4];
        float edge[4];
        float e_select[4];
        float n_edge[4];
        float n_fill[4];
        float n_select[4];
        float selection[4];
    } config;

    /* selection box */
    float sx0, sy0, sx1, sy1;

    /* target x,y,z */
    float tx, ty, tz;
    float angle,vangle,zoom;
    float dangle,dvangle,dzoom;

    bool dragging1,dragging2,dragging3;

    /* node appearance */
    char nodelook[9][32];
    float nodecol[9][4];

    CSFont *nodefont;
public:
	void Init(CSEngine *ep);

	void SetupCamera();
	void DoDraw();

	void OrthoAtSpatial(float,float,float);
	void ReturnToSpace();

	/* current camera setup */
	int c_viewport[4];
    double c_mvmatrix[16],c_projmatrix[16];

    void ResetView();

	void SpaceToScreen(float,float,float, float&, float&);
	void ScreenToSpace(float,float,float&, float&, float&);

	void IncrementFrame();
};


#endif
