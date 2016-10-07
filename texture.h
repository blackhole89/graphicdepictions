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
