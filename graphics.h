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
