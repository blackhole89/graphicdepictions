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
#include "fonts.h"

long utf8unpack(unsigned char *p,long *offs)
{
	long l;

	if(*p & 0x80) {
			l=2;
			if(*p & 0x20) {
				l=3;
				if(*p & 0x10) {
					l=4;
				}
			}
	} else l=1;

	(*offs)+=(l-1);

	switch(l) {
		case 1: return *p;
		case 2: return (*p & ~0xE0)<<6|(*(p+1) & ~0xC0);
		case 3: return (*p & ~0xF0)<<12|(*(p+1) & ~0xC0)<<6|(*(p+2) & ~0xC0);
		case 4: return (*p & ~0xF8)<<18|(*(p+1) & ~0xC0)<<12|(*(p+2) & ~0xC0)<<6|(*(p+3) & ~0xC0);
	}
}

FT_Library CSFont::l=0;

void CSFont::LoadFont(char *fn,int s,int aspace) {
	if(!l) FT_Init_FreeType(&l);
	FT_New_Face(l,fn,0,&f);
	FT_Set_Pixel_Sizes(f,0,s);
	size=s;
	asp=aspace;
}

void CSFont::Render(int tsx,const char* text,int num)
{
	int len=strlen(text);
	int error;
	int x=0,y=1 << (int)ceil((log((double)(2*size))/log(2.0f)));;
	GLuint tex;
	char *buf=new char[y*tsx*4];

	int nrow=0;

    glPushMatrix();
	glPushMatrix(); // this instance will move with newlines

	for (int n=0;n<len&&n<num;++n) {
		if(*(text+n)=='\n') {
			glPopMatrix();
			glTranslatef(0,size+4,0);
			glPushMatrix();
			continue;
		}
		if(*(text+n)<' ') continue;
		FT_UInt glyph_index; /* retrieve glyph index from character code */
		glyph_index = FT_Get_Char_Index( f, utf8unpack((unsigned char*)text+n,(long*)&n) );
		if(!gcache.count(glyph_index)) {
			glyphcacheent e;
			/* load glyph image into the slot (erase previous one) */
			error = FT_Load_Glyph( f, glyph_index, FT_LOAD_DEFAULT );
			if ( error ) continue; /* ignore errors */
			/* convert to an anti-aliased bitmap */
			error = FT_Render_Glyph( f->glyph, FT_RENDER_MODE_NORMAL );
			if ( error ) continue;
			/* now, draw to our target surface */
			//memcpy(buf+(y+i-f->glyph->bitmap_top)*tsx*4+x*4,f->glyph->bitmap.buffer+f->glyph->bitmap.width
			memset(buf,0,y*tsx*4);
			for(int i=0;i<f->glyph->bitmap.rows;++i) {
				for(int j=0;j<f->glyph->bitmap.width;++j) {
					int base;
					int dy;
					base=(j+x+f->glyph->bitmap_left)*4+(dy=(i+size-f->glyph->bitmap_top))*4*tsx;
					buf[base+0]=(dy<(0.7*size/3)||dy>(2.5*size/3))?0xB0:0xFF;
					buf[base+1]=buf[base+0];
					buf[base+2]=0xFF;
					buf[base+3]=f->glyph->bitmap.buffer[j+i*f->glyph->bitmap.width];
				}
			}
			e.advx=(f->glyph->advance.x >> 6) + asp;
			glGenTextures(1,&e.tex);

			glBindTexture(GL_TEXTURE_2D, e.tex);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

			glTexImage2D ( GL_TEXTURE_2D, 0,GL_RGBA, tsx, y, 0,GL_RGBA, GL_UNSIGNED_BYTE, buf);

			gcache[glyph_index]=e;
		}
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, gcache[glyph_index].tex);
		glPushAttrib(GL_CURRENT_BIT);
		glTranslatef(1,1,0);
		glColor3f(0,0,0);
		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex2f(0,0);
			glTexCoord2f(0,1); glVertex2f(0,y);
			glTexCoord2f(1,1); glVertex2f(tsx,y);
			glTexCoord2f(1,0); glVertex2f(tsx,0);
		glEnd();
		glTranslatef(-1,-1,0);
		//glColor3f(1,1,1);
		glPopAttrib();
		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex2f(0,0);
			glTexCoord2f(0,1); glVertex2f(0,y);
			glTexCoord2f(1,1); glVertex2f(tsx,y);
			glTexCoord2f(1,0); glVertex2f(tsx,0);
		glEnd();

		/* increment pen position */
		glTranslatef(gcache[glyph_index].advx,0,0);
	}

	glPopMatrix();
	glPopMatrix();

	delete buf;
}

int CSFont::RenderSingle(char *buf,int shift,int posx,int posy,char **text)
{
	int n=0,error;
	FT_UInt glyph_index; /* retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( f, utf8unpack((unsigned char*)*text,(long*)&n) );
	(*text) += n+1;
	/* load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( f, glyph_index, FT_LOAD_DEFAULT );
	if ( error ) return 0;
	/* convert to an anti-aliased bitmap */
	error = FT_Render_Glyph( f->glyph, FT_RENDER_MODE_NORMAL );
	if ( error ) return 0;
	/* now, draw to our target surface */
	for(int i=0;i<f->glyph->bitmap.rows;++i) {
		for(int j=0;j<f->glyph->bitmap.width;++j) {
			int base;
			int dy;
			base=(j+posx+f->glyph->bitmap_left)*4+(i+posy-f->glyph->bitmap_top)*4*shift;
			dy=i+size-f->glyph->bitmap_top;
			buf[base+0]=(dy<(1.7*size/3)||dy>(2.5*size/3))?0xB0:0xFF;
			buf[base+1]=buf[base+0];
			buf[base+2]=0xFF;
			buf[base+3]=f->glyph->bitmap.buffer[j+i*f->glyph->bitmap.width];
		}
	}
	/* increment pen position */
	return (f->glyph->advance.x >> 6) + asp;
}

void CSTextContainer::IncrementFrame()
{
	if(!*ptr) return;	//text over
	if(*ptr == '\001') return; //wait for confirmation;
	--timeout;
	while(!timeout && *ptr) {
		if(*ptr == '\n') {
			//newline.
			//xpos=0;
			//ypos+=ft->size+4;
			++ptr;
			timeout=default_timeout;
		} else if(*ptr == '\002') {
		    ++ptr;
		    timeout=2;
		} else {
			int n=0;
			if(snd && (*ptr!=' ')) s.s->PlaySound("CHAR");
			utf8unpack((unsigned char*)ptr,(long*)&n);
			ptr+=(n+1);
			timeout=default_timeout;
			//xpos+=ft->RenderSingle(buf,sx,xpos,ypos,&ptr);
		}
	}
	//glBindTexture(GL_TEXTURE_2D, texture);

	//glTexImage2D(GL_TEXTURE_2D,0, GL_RGBA, sx, sy, 0,GL_RGBA, GL_UNSIGNED_BYTE, buf);*/
}

void CSTextContainer::SendConfirm()
{
	if(*ptr == '\001') ++ptr;
}

bool CSTextContainer::IsDone()
{
	return !*ptr;
}

bool CSTextContainer::IsWaiting()
{
	return (*ptr == '\001');
}

void CSTextContainer::Render()
{
	/*glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, texture);
	glColor3f(0,0,0);
	glTranslatef(2,2,0);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2f( 0, 0);
		glTexCoord2f(1,0); glVertex2f(sx, 0);
		glTexCoord2f(1,1); glVertex2f(sx,sy);
		glTexCoord2f(0,1); glVertex2f( 0,sy);
	glEnd();
	glTranslatef(-2,-2,0);
	glColor3f(1,1,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2f( 0, 0);
		glTexCoord2f(1,0); glVertex2f(sx, 0);
		glTexCoord2f(1,1); glVertex2f(sx,sy);
		glTexCoord2f(0,1); glVertex2f( 0,sy);
	glEnd();
	*/
	pthread_mutex_lock(&mutex);
	if(ft->size<16) ft->Render(16,txt,ptr-txt);
	else ft->Render(64,txt,ptr-txt);
	pthread_mutex_unlock(&mutex);
}

void CSTextContainer::AppendText(char *text)
{
	pthread_mutex_lock(&mutex);
	timeout=1;
	memcpy(txt+strlen(txt),text,strlen(text)+1);
	pthread_mutex_unlock(&mutex);
}

void CSTextContainer::ReplaceText(char *text)
{
	pthread_mutex_lock(&mutex);
	memcpy(txt,text,strlen(text)+1);
	ptr=txt;
	xpos=0; ypos=ft->size;
	timeout=1;
	memset(buf,0,sx*sy*4);
	pthread_mutex_unlock(&mutex);
}

void CSTextContainer::MaybeReplaceText(char *text)
{
    if(!strcmp(txt,text)) return;
	pthread_mutex_lock(&mutex);
	memcpy(txt,text,strlen(text)+1);
	ptr=txt;
	xpos=0; ypos=ft->size;
	timeout=1;
	memset(buf,0,sx*sy*4);
	pthread_mutex_unlock(&mutex);
}

void CSTextContainer::Initialize(int sx_,int sy_,int bufs_,int spd,CSFont *font,bool usesnd)
{
	//copy various data;
	sx=sx_;
	sy=sy_;
	bufs=bufs_;
	txt=new char[bufs];
	ptr=txt;
	*txt=0;
	buf=new char[sx*sy*4];
	memset(buf,0,sx*sy*4);
	ft=font;
	xpos=0; ypos=font->size;
	default_timeout=spd;
	timeout=1;
	snd=usesnd;

	#ifndef WIN32
	pthread_mutex_init(&mutex,NULL);
	#else
	mutex=PTHREAD_MUTEX_INITIALIZER;
    #endif

	glGenTextures(1,&texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}
