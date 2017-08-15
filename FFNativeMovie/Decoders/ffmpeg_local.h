#pragma once


#include "./ffmpeg_common.h"
#include "ffmpeg.h"


//<! ���ز�����

class FfmpegLocal : public Ffmpeg
{

public:
	FfmpegLocal();
	~FfmpegLocal();

	virtual int32_t GetCachedPosition(uint64_t * position)  { *position = 0;  return -1; }; //<!��ȡ��ǰ���ڻ����ʱ��  ������Ƶ�޻��壬����-1��
	virtual int32_t GetIsBuffering() { return 0; }; //<!��ȡ��ǰ�Ƿ����ڻ���    ������Ƶ�޻��壬����0��

private:
	//static int AudioThread(void* ptr);
	//static int VideoThread(void* ptr);
	static int ReadThread(void* ptr);

	virtual int32_t InitThread();

};