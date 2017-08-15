#pragma once


#include "./ffmpeg_common.h"


class Ffmpeg;

/*!
* \class： SESSION
*
* \brief ： Unity播放器相关参数
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
 * \brief: 本地和网络播放器的父类
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
	    static Uint32 SDL_Timer_Cb(Uint32 interval, void* param);   //<! 外部时钟的回调函数，效果不好，停用 !>
	    static int ExtClockThread(void* ptr);    //<! 外部时钟的线程，效果不好，停用 !>
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


	std::string GetDecoderName() { return std::string("ffmpeg"); }   //<!获取当前解码器的名称
	virtual int32_t Play();
	virtual int32_t Pause();
	virtual int32_t Stop(); //<!停止 并释放资源
	virtual int32_t GetPosition(uint64_t * position); //<!获取当前正在播放的时间
	virtual int32_t VolumeUp(int32_t vo); //<!提升音量， vo ： 0~100
	virtual int32_t VolumeDown(int32_t vo); //<!降低音量， vo ： 0~100
	virtual int32_t Open(std::string fileName); //<!打开文件或url

	virtual int64_t GetFilmDuration();//<! 获取影片时长， in ticks
	virtual int32_t SetCyclePlay(bool); //<! 设置是否循环播放
	virtual int32_t GetFrame(SESSION* session, ID3D11Resource* resource); //<!获取一帧显存图像
	virtual int32_t SetVolume(int32_t vo);  //<! 设置音量 ， vo：0~100
	void SetFilePath(std::string fn); //<!设置文件路径
	std::string GetFileName();  //<!获取文件路径
	void SetHWND(HWND hwnd);  //<!设置SDL窗口句柄
	HWND GetHWND();       //<!获取当前SDL窗口句柄
	void SetWindowSize(uint32_t width, uint32_t height);  //<!设置窗口尺寸
	int32_t ChooseSynMethod();  //<! 确定同步方式 !>
	uint32_t GetMovieWidth();  //<!获取影片的宽
	uint32_t GetMovieHeight(); //<!获取影片的高

	uint32_t GetLastFrameCount(); //<! 获取解码多少帧数据
	uint32_t SeekTo(uint64_t position, int32_t isBufferClean, int32_t isPlayProgressBarRefresh = PlayProgressBarFlag::REFRESH_PROGRESSBAR);
	virtual int32_t GetCachedPosition(uint64_t * position) = 0; //<!获取当前正在播放的时间
	virtual int32_t GetIsBuffering() = 0; //<!获取当前是否在缓冲

	int32_t SetMovieReplay(ReplayType replayType);
	//<!Start
	//<! 从private移到public, 为了在子类的线程中使用
	void ClearAudioQueue(AudioQueue* aq);
	void ClearVideoQueue(VideoQueue* vq);
	int32_t CreateVideoItemFromPacket(AVPacket*);   //<!创建视频队列Item
	double SynchronizeVideo(FfVideoState *is, AVFrame *src_frame, double pts);
	double SynchronizeAudio(FfVideoState *is, AVFrame *src_frame, double pts);
	int32_t CreateAudioItemFromPacket(AVPacket*); //<!创建音频队列Item
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
	virtual int32_t InitDecoder();  //<!初始化解码器
	virtual int32_t ReleaseDecoder();
	int32_t ReleaseSources();
	double SecondsToTicks(double sec);
	double TicksToSeconds(double ticks);
	int32_t GetIsPlayFinished();
	SDL_Rect RecalculateWidthHeightRatio(FfVideoState*);
	//int32_t LockAudio();   //<! 暂停声音callback，否则缓冲或者暂停时，某些视频有杂音 !>
	//int32_t UnlockAudio();
	//<!End

private:
	//Ffmpeg();
	int32_t InitSDL();  //<!  初始化SDL
	int32_t InitD3d11();  //<!初始化显存和D3D11相关资源
	int32_t InitPicoHMD();
	int32_t ReleasePicoHMD();

	int32_t InitGPUBuffers();
	void CalcFilmInfos();
	virtual int32_t InitThread() = 0;  //<! 纯虚函数 ， 本地和网络播放器线程数不同
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
	uint32_t m_iWndWidth, m_iWndHeight;//<! SDL窗口的宽 //<! SDL窗口的高
	
public:
	FfVideoState* m_pVideoState = NULL;    //<! 用于播放显示相关的标志，实例等

	SDL_Thread* m_pReadThread; // <! 读取线程
	SDL_Thread* m_pVideoThread; //<! 视频播放线程
	SDL_Thread* m_pAudioThread; //<! 音频播放线程
#if USE_SDL_VIDEO
	SDL_Thread* m_pSDLVideoThread; //<! 音频播放线程
#endif
	//SDL_Thread* m_pExtClockThread; //<! 外部时钟线程    效果不好，停用
};


