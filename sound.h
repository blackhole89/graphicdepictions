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

#ifndef _SOUND_H_
#define _SOUND_H_

#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#ifdef WIN32
#include "I:\oalsdk\include\al.h"
#include "I:\oalsdk\include\alc.h"
#include "vorbis/vorbisfile.h"
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <vorbis/vorbisfile.h>
#endif

class CSSStream {
protected:
	ALuint source;
	ALuint buffers[16];

	OggVorbis_File *f;
	int len,len0;

	short buf[4096];

	bool playing;
public:
	void Init(char *filename);
	void Handle();

	void Play();
	void Stop();
};

class CSSoundSystem {
protected:
	ALuint sources[16];
	int nextsource;
	ALuint bgm0;
	ALuint step;
	ALCcontext *ctx;

	std::map<std::string,ALuint> bank;

	std::map<std::string,CSSStream*> streams;
public:
	void Init();
	void Load(char *fname,ALuint *buf);

	void PlayBGM();
	void PlayStep(int px,int py);
	void LoadSound(std::string name,char *fname);
	void PlaySound(std::string name);

	void Handle();
};

#endif
