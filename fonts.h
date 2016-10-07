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
