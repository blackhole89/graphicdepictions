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

#ifndef _FONTS_H_
#define _FONTS_H_

#include <ft2build.h>
#include <string.h>
#include <map>
#include FT_FREETYPE_H

struct glyphcacheent {
	int advx;
	GLuint tex;
};

class CSFont {
public:
	static FT_Library l;
	FT_Face f;
	int size;
	int asp;

	std::map<FT_UInt,glyphcacheent> gcache;

	void LoadFont(char *fn,int s,int aspace);
	void Render(int tsx,const char* text,int num);
	int RenderSingle(char *buf,int shift,int posx,int posy,char **text);
};

class CSTextContainer {
protected:
	CSFont *ft;
	int sx,sy,bufs,xpos,ypos;
	char *buf;
	char *txt,*ptr;
	GLuint texture;
	int timeout;
	int default_timeout;
	bool snd;

	pthread_mutex_t mutex;
public:
	void IncrementFrame();
	void SendConfirm();
	void Render();
	void AppendText(char *text);
	void ReplaceText(char *text);
	void MaybeReplaceText(char *text);
	void Initialize(int sx,int sy,int bufs,int spd,CSFont *ft,bool usesnd=false);
	bool IsDone();
	bool IsWaiting();
};

#endif
