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
