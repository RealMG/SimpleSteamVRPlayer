#pragma once


#include "./ffmpeg_common.h"


class Ffmpeg;

/*!
* \class�� SESSION
*
* \brief �� Unity��������ز���
*
* \author nico.liu
* \date 2017-1-11 10:50:43
*/

struct SESSION {
	Ffmpeg *player;
	HANDLE waitHandle;
	//ID3D11Resource *leftEyeTexture;
	//ID3D11Resource *rightEyeTexture;
	//std::mutex *frameAccess;
	int32_t lastFrameUploaded;
	Video3DMode mode;
};






/*!
 * \class: Ffmpeg
 *
 * \brief: ���غ����粥�����ĸ���
 *
 * \author nico.liu
 * \date 2017-1-11 10:49:48
 */

class Ffmpeg
{

public:
	Ffmpeg();
	~Ffmpeg();


	//static void SDL_AudioCallBack(void *, uint8_t *, int);

	/*
	    static Uint32 SDL_Timer_Cb(Uint32 interval, void* param);   //<! �ⲿʱ�ӵĻص�������Ч�����ã�ͣ�� !>
	    static int ExtClockThread(void* ptr);    //<! �ⲿʱ�ӵ��̣߳�Ч�����ã�ͣ�� !>
		static int AudioThread(void* ptr);
		static int VideoThread(void* ptr);
		static int ReadThread(void* ptr);
		*/

	static int AudioThread(void* ptr);
	void SubmitToHMD(ID3D11Texture2D* leftTex, ID3D11Texture2D* rightTex, D3D11_BOX* texCopyRegion, int32_t noScale);
	static int VideoThread(void* ptr);
	static int SDLVideoThread(void* ptr);
	static int LockMgr(void** mtx, enum AVLockOp op);
	static int InterruptCallBack(void* usrdata);


	std::string GetDecoderName() { return std::string("ffmpeg"); }   //<!��ȡ��ǰ������������
	virtual int32_t Play();
	virtual int32_t Pause();
	virtual int32_t Stop(); //<!ֹͣ ���ͷ���Դ
	virtual int32_t GetPosition(uint64_t * position); //<!��ȡ��ǰ���ڲ��ŵ�ʱ��
	virtual int32_t VolumeUp(int32_t vo); //<!���������� vo �� 0~100
	virtual int32_t VolumeDown(int32_t vo); //<!���������� vo �� 0~100
	virtual int32_t Open(std::string fileName); //<!���ļ���url

	virtual int64_t GetFilmDuration();//<! ��ȡӰƬʱ���� in ticks
	virtual int32_t SetCyclePlay(bool); //<! �����Ƿ�ѭ������
	virtual int32_t GetFrame(SESSION* session, ID3D11Resource* resource); //<!��ȡһ֡�Դ�ͼ��
	virtual int32_t SetVolume(int32_t vo);  //<! �������� �� vo��0~100
	void SetFilePath(std::string fn); //<!�����ļ�·��
	std::string GetFileName();  //<!��ȡ�ļ�·��
	void SetHWND(HWND hwnd);  //<!����SDL���ھ��
	HWND GetHWND();       //<!��ȡ��ǰSDL���ھ��
	void SetWindowSize(uint32_t width, uint32_t height);  //<!���ô��ڳߴ�
	int32_t ChooseSynMethod();  //<! ȷ��ͬ����ʽ !>
	uint32_t GetMovieWidth();  //<!��ȡӰƬ�Ŀ�
	uint32_t GetMovieHeight(); //<!��ȡӰƬ�ĸ�

	uint32_t GetLastFrameCount(); //<! ��ȡ�������֡����
	uint32_t SeekTo(uint64_t position, int32_t isBufferClean, int32_t isPlayProgressBarRefresh = PlayProgressBarFlag::REFRESH_PROGRESSBAR);
	virtual int32_t GetCachedPosition(uint64_t * position) = 0; //<!��ȡ��ǰ���ڲ��ŵ�ʱ��
	virtual int32_t GetIsBuffering() = 0; //<!��ȡ��ǰ�Ƿ��ڻ���

	int32_t SetMovieReplay(ReplayType replayType);
	//<!Start
	//<! ��private�Ƶ�public, Ϊ����������߳���ʹ��
	void ClearAudioQueue(AudioQueue* aq);
	void ClearVideoQueue(VideoQueue* vq);
	int32_t CreateVideoItemFromPacket(AVPacket*);   //<!������Ƶ����Item
	double SynchronizeVideo(FfVideoState *is, AVFrame *src_frame, double pts);
	double SynchronizeAudio(FfVideoState *is, AVFrame *src_frame, double pts);
	int32_t CreateAudioItemFromPacket(AVPacket*); //<!������Ƶ����Item
	uint32_t GetItemFromAudioQueue(AudioQueue* aq, AudioQueueItem** item);
	uint32_t GetItemFromVideoQueue(VideoQueue* vq, VideoQueueItem** item);
	int32_t SetGPUMemEnable(PictureTexture* texturePointer);
	PictureTexture* FindEnabledGPUMem();
	int32_t InitAudioQueue(AudioQueue** aq);
	void	 ReleaseAudioQueue(AudioQueue** aq);
	uint32_t PutItemIntoAudioQueue(AudioQueue* aq, AudioQueueItem* item);
	int32_t InitVideoQueue(VideoQueue** vq);
	void ReleaseVideoQueue(VideoQueue** vq);
	uint32_t PutItemIntoVideoQueue(VideoQueue* vq, VideoQueueItem* item);
	double GetPts(AVPacket* packet, AVFrame* frame, AVRational timeBase);
	int32_t FirstFrameCheck(FirstFrame* first_frame, double pts);
	int32_t GetDelay(FfVideoState* Ffvs);
	virtual int32_t InitDecoder();  //<!��ʼ��������
	virtual int32_t ReleaseDecoder();
	int32_t ReleaseSources();
	double SecondsToTicks(double sec);
	double TicksToSeconds(double ticks);
	int32_t GetIsPlayFinished();
	SDL_Rect RecalculateWidthHeightRatio(FfVideoState*);
	//int32_t LockAudio();   //<! ��ͣ����callback�����򻺳������ͣʱ��ĳЩ��Ƶ������ !>
	//int32_t UnlockAudio();
	//<!End

private:
	//Ffmpeg();
	int32_t InitSDL();  //<!  ��ʼ��SDL
	int32_t InitD3d11();  //<!��ʼ���Դ��D3D11�����Դ
	int32_t InitPicoHMD();
	int32_t ReleasePicoHMD();

	int32_t InitGPUBuffers();
	void CalcFilmInfos();
	virtual int32_t InitThread() = 0;  //<! ���麯�� �� ���غ����粥�����߳�����ͬ
	int32_t InitSDLVideo();
	int32_t InitSDLAudio(uint64_t channellayout, int32_t channels, int32_t samplerate, int32_t samples);
	int32_t InitDecoderVideo();
	int32_t InitDecoderAudio();
	
	double CalculateDifferTime(double currentVideoPts, double currentAudioPts);
	int32_t ReleaseD3D11();
	int32_t ReleaseGPUBuffers();
	int32_t ReleaseSDL();


private:
	std::string m_sFilename;
	HWND m_hHwnd;
	uint32_t m_iWndWidth, m_iWndHeight;//<! SDL���ڵĿ� //<! SDL���ڵĸ�
	
public:
	FfVideoState* m_pVideoState = NULL;    //<! ���ڲ�����ʾ��صı�־��ʵ����

	SDL_Thread* m_pReadThread; // <! ��ȡ�߳�
	SDL_Thread* m_pVideoThread; //<! ��Ƶ�����߳�
	SDL_Thread* m_pAudioThread; //<! ��Ƶ�����߳�
#if USE_SDL_VIDEO
	SDL_Thread* m_pSDLVideoThread; //<! ��Ƶ�����߳�
#endif
	//SDL_Thread* m_pExtClockThread; //<! �ⲿʱ���߳�    Ч�����ã�ͣ��
};


