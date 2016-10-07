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
#include "file.h"

void CSFile::Open(char *fn)
{
    char buf[256];
    sprintf(buf,"data/%s",fn);
	fl=fopen(buf,"rb");
}

int CSFile::GetBytes(void *buf,int n)
{
	return fread(buf,n,1,fl);
}

int CSContainer::GetBlock()
{
	int ret;
	if(!GetBytes(&ret,4)) return -1;
	GetBytes(&lastblksize,4);
	return ret;
}

int CSContainer::SkipBlock()
{
	fseek(fl,lastblksize,SEEK_CUR);
	return lastblksize;
}

int CSContainer::GetCurrentBlockSize()
{
	return lastblksize;
}

void CSOutContainer::Open(char *fn)
{
	fl=fopen(fn,"w+b");
}

void CSOutContainer::OpenExisting(char *fn)
{
    fl=fopen(fn,"r+b");
}

void CSOutContainer::Rewind()
{
    fseek(fl,0,SEEK_SET);
}

void CSOutContainer::InitBlock(long code)
{
	fwrite(&code,4,1,fl);
	fwrite(&code,4,1,fl);
	cblocksize=0;
}

void CSOutContainer::NextBlock(long code)
{
    int c,d;
    while(!feof(fl) && c!=code) {
        fread(&c,4,1,fl);
        printf("%08X\n",c);
        if(feof(fl)) break;
        fread(&d,4,1,fl);
        printf(":%08X\n",d);
        if(c!=code) fseek(fl,d,SEEK_CUR);
    }
    if(feof(fl)) {
        clearerr(fl);
        InitBlock(code);
    }
    cblocksize=0;
}

int CSOutContainer::FinishBlock()
{
	fseek(fl,-cblocksize-4,SEEK_CUR);
	fwrite(&cblocksize,4,1,fl);
	fseek(fl,cblocksize,SEEK_CUR);
	int k=cblocksize;
	cblocksize=0;
	return cblocksize;
}

void CSOutContainer::PutBytes(char *buf,int n)
{
	fwrite(buf,n,1,fl);
	cblocksize+=n;
}

void CSOutContainer::PutLong(long n)
{
	fwrite(&n,4,1,fl);
	cblocksize+=4;
}
