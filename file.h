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

#ifndef _FILE_H_
#define _FILE_H_

#include "stdio.h"
#include "stdafx.h"

class CSFile {
protected:
	FILE *fl;
public:
	void Open(char* fn);
	int GetBytes(void *buf,int n);
};

class CSContainer : public CSFile {
public:
	int lastblksize;

	int SkipBlock();
	int GetBlock();
	int GetCurrentBlockSize();
};

class CSOutContainer {
protected:
	FILE *fl;
	int cblocksize;
public:
	void Open(char *fn);
	void OpenExisting(char *fn);
	void Rewind();
	void NextBlock(long code);
	void InitBlock(long code);
	int FinishBlock();
	void PutBytes(char *buf,int n);
	void PutLong(long n);
};

#endif
