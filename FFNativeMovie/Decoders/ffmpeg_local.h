#pragma once


#include "./ffmpeg_common.h"
#include "ffmpeg.h"


//<! 本地播放器

class FfmpegLocal : public Ffmpeg
{

public:
	FfmpegLocal();
	~FfmpegLocal();

	virtual int32_t GetCachedPosition(uint64_t * position)  { *position = 0;  return -1; }; //<!获取当前正在缓冲的时间  本地视频无缓冲，返回-1；
	virtual int32_t GetIsBuffering() { return 0; }; //<!获取当前是否正在缓冲    本地视频无缓冲，返回0；

private:
	//static int AudioThread(void* ptr);
	//static int VideoThread(void* ptr);
	static int ReadThread(void* ptr);

	virtual int32_t InitThread();

};