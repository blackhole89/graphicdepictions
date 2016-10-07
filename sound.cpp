#include "stdafx.h"
#include "sound.h"

#include "stdafx.h"
#include "sound.h"

void CSSStream::Init(char *filename)
{
	alGenBuffers(16, buffers);

	alGenSources(1, &source);
	alSourcef(source, AL_GAIN, 0.8);
	alSourcei(source, AL_LOOPING, AL_FALSE);
	alSource3f(source, AL_POSITION, 0.0, 2.0, 0.0);

	if(strstr(filename,".ogg")) {
		int bs;

		FILE *fl=fopen(filename,"rb");
		f=new OggVorbis_File;
		ov_open_callbacks(fl,f,NULL,0,OV_CALLBACKS_DEFAULT);

		len=ov_pcm_total(f,-1);

		short *data=new short[len*2];

		len*=4;
		len0=len;

		for(int i=0;i<16;++i) {
			int l;
			len-=(l=ov_read(f,(char*)buf,8192,0,2,1,&bs));
			alBufferData(buffers[i], AL_FORMAT_STEREO16, buf, l, 44100);
		}

		alSourceQueueBuffers(source, 16, buffers);
	}
	playing=false;
}

void CSSStream::Handle()
{
	int val;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
	if(!val) return;
	while(val--) {
		ALuint buffer; int l,bs=0;
		if(!len) {
			ov_pcm_seek(f,0);
			len=len0;
		}
		l=ov_read(f,(char*)buf,8192,0,2,1,&bs);
		len-=(l+=ov_read(f,((char*)buf)+l,8192-l,0,2,1,&bs));
		alSourceUnqueueBuffers(source, 1, &buffer);
        alBufferData(buffer, AL_FORMAT_STEREO16, buf, l, 44100);
        alSourceQueueBuffers(source, 1, &buffer);
	}
	alGetSourcei(source, AL_SOURCE_STATE, &val);
    if(playing && val!=AL_PLAYING)
		alSourcePlay(source);

}

void CSSStream::Play()
{
	alSourcePlay(source);
	playing=true;
}

void CSSStream::Stop()
{
	alSourceStop(source);
	playing=false;
}

void CSSoundSystem::Init()
{
    ALCdevice *dev;
	alcMakeContextCurrent(ctx=alcCreateContext(dev=alcOpenDevice(NULL),NULL));

	alGenSources(16, sources);

	float o[6]={0.0,1.0,0.0,0.0,0.0,1.0};
	alListener3f(AL_POSITION,0,0,0);
	alListenerfv(AL_ORIENTATION,o);
	alListener3f(AL_VELOCITY,0,0,0);
	alListenerf(AL_GAIN,1.0);

	/*CSSStream *s=new CSSStream;
	s->Init("data/sptest_4.ogg");

	streams["bgm00"]=s;

	//Load("sptest_4.ogg",&bgm0);
	Load("step.raw",&step);
	LoadSound("CHAR","key.raw");*/

	nextsource=2;
}

void CSSoundSystem::Handle()
{
	for(std::map<std::string,CSSStream*>::iterator i=streams.begin();i!=streams.end();++i) {
		i->second->Handle();
	}
}

void CSSoundSystem::Load(char *fname,ALuint *buf)
{
    char fbuf[256];
    sprintf(fbuf,"data/%s",fname);
    fname=fbuf; //TODO: improve

	if(strstr(fname,".ogg")) {
		OggVorbis_File *f;
		int bs;
		int len,len0;

		FILE *fl=fopen(fname,"rb");
		f=new OggVorbis_File;
		ov_open_callbacks(fl,f,NULL,0,OV_CALLBACKS_DEFAULT);

		len=ov_pcm_total(f,-1);

		short *data=new short[len*2];

		len*=4;
		len0=len;

		while(len) len-=ov_read(f,((char*)data)+(len0-len),len,0,2,1,&bs);

		alGenBuffers(1, buf);
		alBufferData(*buf, AL_FORMAT_STEREO16, data, len,44100);
	} else {
		struct stat st;
		int a=open(fname,O_RDONLY);
		// load sound as raw
		FILE *fl=fopen(fname,"rb");
		fstat(a,&st);
		short *data=new short[st.st_size/2];
		fread(data,st.st_size,1,fl);
		fclose(fl);
		close(a);

		alGenBuffers(1, buf);
		alBufferData(*buf, AL_FORMAT_STEREO16, data, st.st_size,44100);
	}
}

void CSSoundSystem::PlayBGM()
{
/*	alSourcei(sources[0], AL_BUFFER, bgm0);
	alSourcef(sources[0], AL_GAIN, 0.8);
	alSourcei(sources[0], AL_LOOPING, AL_TRUE);
	alSource3f(sources[0], AL_POSITION, 0.0, 2.0, 0.0);
	alSourcePlay(sources[0]);
*/
	streams["bgm00"]->Play();
}

void CSSoundSystem::PlayStep(int px,int py)
{
	alSourcei(sources[1], AL_BUFFER, step);
	alSourcei(sources[1], AL_LOOPING, AL_FALSE);
	alSourcef(sources[1], AL_GAIN, 0.5);
	alSource3f(sources[1], AL_POSITION, (1/256.0)*(px-256.0), 1.0,0);
	alSource3f(sources[1], AL_VELOCITY, 0,0,0);
	alSourcePlay(sources[1]);
}

void CSSoundSystem::LoadSound(std::string name,char *fname)
{
	Load(fname,&(bank[name]));
}

void CSSoundSystem::PlaySound(std::string name)
{
	if(!bank.count(name)) return;
	int state;
	do {
		++nextsource;
		if(nextsource==16) nextsource=2;
		alGetSourcei(sources[nextsource],AL_SOURCE_STATE,&state);
	} while(state==AL_PLAYING);
	alSourcei(sources[nextsource], AL_BUFFER, bank[name]);
	alSourcei(sources[nextsource], AL_LOOPING, AL_FALSE);
	alSourcef(sources[nextsource], AL_GAIN, 0.5);
	alSource3f(sources[nextsource], AL_POSITION, 0.0, 1.0,0);
	alSource3f(sources[nextsource], AL_VELOCITY, 0,0,0);
	alSourcePlay(sources[nextsource]);
}
