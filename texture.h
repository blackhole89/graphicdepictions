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

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#ifdef _WIN32
#include <olectl.h>
#endif
#include <math.h>
#include "stdafx.h"

struct BMPImage
{
    int   width;
    int   height;
    char *data;
};

class CSTexture {
public:
	unsigned char *data;
	int xs;
	int ys;

	bool alpha;

	GLuint tid;

	void LoadBitmap(char* filename);
	void BuildPOT(BMPImage *img);

	void CKCopy(CSTexture *src,int r,int g,int b);
	void SubareaCopy(CSTexture *src,int x1,int y1,int x2,int y2);

	void Bind();

	char *name;
};

#endif
