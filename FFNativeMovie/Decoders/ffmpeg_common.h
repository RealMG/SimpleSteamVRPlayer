#pragma once

#pragma warning(disable:4996)
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avformat.lib")


extern "C"
{
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
	//#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
}

#include "../stdafx.h"
#include <string>
#include <D3D11.h>
#include <map>
#include "PicoPlugin.h"


#include "D3Dx11tex.h"  
#pragma comment(lib, "D3DX11.lib")  



#define USE_SDL_VIDEO 0

#define	AV_FORMAT_CURRENT_USED     AV_PIX_FMT_RGBA
#define	SDL_FORMAT_CURRENT_USED    SDL_PIXELFORMAT_ABGR8888


//#define	AV_FORMAT_CURRENT_USED     AV_PIX_FMT_YUV420P
//#define	SDL_FORMAT_CURRENT_USED   SDL_PIXELFORMAT_IYUV
#define RENDER_TARGET_TEXTURE_NUM 3

#define MAX_GPU_BUFFERS 60

#define VideoBufferMaxSize MAX_GPU_BUFFERS//  //视频缓冲区最大值，大于此值 则不下载数据
#define VideoBufferMinSize 1//  //视频缓冲区最小值，小于此值，则唤醒下载

#define AudioBufferMaxSize 500//  //音频缓冲区最大值，大于此值 则不下载数据
#define AudioBufferMinSize 1//  //音频缓冲区最小值，小于此值，则唤醒下载

#define CacheBufferMaxSize 65534// FAT16文件系统每个文件夹下最多的文件个数，FAT32和NTFS比这个要多，但以可能的最小值为准。 
#define CacheBufferMinSize 30//  


#define MAX_CACHE_SPACE_IN_KB 100<<11   //<! 200MB !> 
#define MIN_FILE_SIZE_IN_BYTES 4<<10 //<! 4KB !>


#define MIN_CACHE_FILE_COUNT 100  //<! 缓存文件小于此值时，暂停播放 !>
#define MIN_BUFFER_STATUS_FRAME_COUNT 500 //<! 缓存过后，可以继续播放的最少文件数 !>

#define MAX_VIDEO_DIFFER_TIME 0.2 

#define MAX_VIDEO_AUDIO_DIFFER_TIME 0.3  //<! 音视频进行同步的上限 !>
#define MIDDLE_VIDEO_AUDIO_DIFFER_TIME 0.15   //<! 音视频进行同步的中限 !>
#define MIN_VIDEO_AUDIO_DIFFER_TIME 0.04   //<! 音视频进行同步的下限 !>

#define MAX_VIDEO_DIFFER_ERROR 60   //<! 视频时间戳出错差值，大于这个值，切换同步方式 !>

#define DECODER_THREAD_COUNT 8

#define STREAM_TRACK_EMPTY -1

#define PACKET_THROW_COUNT_AFTER_SEEK 10

#define THREAD_CON_WAIT_TIMEOUT 500   //<! 线程的等待超时时间 !>

#define DEFAULT_WINDOW_WIDTH 1024

#define DEFAULT_WINDOW_HEIGHT 768

#define SDL_THREAD_WAITTIMEOUT 100


/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 60

//存储音频的队列
typedef struct AudioQueueItem
{
	uint8_t *AudioData;//音频数据
	int32_t Length;//音频长度
	double Pts;//时间戳
	AudioQueueItem *Next;//尾部
}AudioQueueItem;

typedef struct AudioQueue
{
	AudioQueueItem *FirstItem;//队列头
	AudioQueueItem *LastItem;//队列位
	int32_t Length;//队列长度

	SDL_mutex *audioMutex;//用于同步两个线程同时操作队列的 互斥量
	SDL_cond *audioCond;//唤醒线程

}AudioQueue;




typedef struct _PictureTexture
{
	RenderTexture* rt;
#if USE_SDL_VIDEO
	uint8_t* raw_pixs;
#endif
}PictureTexture;



//存储视频的队列
typedef struct VideoQueueItem
{
	PictureTexture* Texture;//视频数据
	int32_t Width;//视频图片的宽度
	int32_t Height;//视频图片的高度
	int32_t Length;//视频长度
	double Pts;//时间戳
	VideoQueueItem *Next;//尾部
}VideoQueueItem;

typedef struct VideoQueue
{
	VideoQueueItem *FirstItem;//队列头
	VideoQueueItem *LastItem;//队列位
	int32_t Length;//队列长度
	double BufferPts;//缓冲的pts

	SDL_mutex *videoMutex;//用于同步两个线程同时操作队列的 互斥量
	SDL_cond *videoCond;//唤醒线程

}VideoQueue;


typedef struct FirstFrame
{
	enum IsFirstFrame isFirstFrame;  //<! 判断第一帧，m3u8文件，第一帧取出的时间戳可能不是0，因此显示和seek需要做偏移 !>
	double FirstFramePts;    //<! 第一帧的时间，或者定义为需要偏移的时间 !
}FirstFrame;



typedef struct AudioParams {
	int32_t freq;
	int32_t channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
	int32_t frame_size;
	int32_t bytes_per_sec;
	int32_t nb_samples;
} AudioParams;



typedef struct FfVideoState{

	double m_iSeekTime;  //<! 需要跳转到的时间
	int32_t m_iCurrentVolume;//<! 当前的音量 

	//int32_t m_iPacketThrowCount;    //<! packet丢弃计数，seek后需要丢弃几帧数据来保证时间正确 !>


	enum CacheFinishedFlag m_iCacheFinishedFlag;//<! 是否已缓冲到最后 !>
	enum PlayStatus m_iPlayingFlag;// <! 播放状态enum PlayStatusFlag;
	enum BufferCleanFlag m_iBufferCleanFlag;//<!是否需要清空缓存。当seek时需要，循环播放时不需要清空
	enum ReplayType m_iMovieReplayFlag;//<!是否需要重复播放                                                                                          //<! 自动重复播放，当前无效 !>
	enum SeekSuccessFlag m_iIsSeekSuccessFlag;//<!seek时是否成功，若不成功一次，不重复seek ，      //<! 自动重复播放，当前无效 !>
	enum NetworkStatus m_iNetworkStatusFlag;//<! 网络状态返回值 !>
	enum NetworkBufferingStatus m_iNetworkBufferingStatusFlag;//<! 网络是否正在缓冲返回值 !>
	enum PlayFinishedFlag m_iPlayFinishedFlag;//<! 是否播放到最后 !>
	enum PlayProgressBarFlag m_iPlayProgressBarRefreshFlag;//<!是否需要更新播放进度条。当暂停状态缓冲完进行跳转时，不需要更新      //<! 自动重复播放，当前无效 !>
	enum SeekInCacheFlag m_iSeekInCacheFlag;  //<! 在缓存中跳转，只对Online播放器有效
	enum SynMethod m_iSynMethod;//<! 同步方式
	enum SynMethod m_iFirstSynMethodSelected;//<! 保存第一次选择的同步方式
	enum SeekStatus m_iSeekFlag;  //<! seek 状态 !>

	//FirstFrame m_sFirstAudioFrame;
	//FirstFrame m_sFirstVideoFrame;
	FirstFrame m_sFirstFrame;   //<! 网络视频，用第一包的时间当做偏移时间 !>

	//ExtClock m_sExtClock;   //<! 外部时钟，效果不好停用 !>

	//double m_dLastDisplayPosition;
	double m_dCurrentPosition;
	double m_dCurrentVideoPts; //<! 当前视频时间戳
	double m_dCurrentAudioPts; //<! 当前音频时间戳
	int64_t m_dFilmDuration; //<! 影片长度
	double m_dFilmDurationInSecs; //<! 转换为秒后的影片长度

	double m_dCurrentDecodeVideoPts; //<! 解码的视频时间戳 !>
	double m_dCurrentDecodeAudioPts; //<! 解码的音频时间戳 !>

	double m_dCurrentCacheVideoPts; //<! 缓冲视频时间戳
	double m_dCurrentCacheAudioPts; //<! 缓冲音频时间戳
	double m_dCurrentCachePosition;//<! 缓冲到的位置 !>
	uint64_t m_ui64CurrentCacheLength; //<! 缓冲队列的长度 !>

	AVFormatContext *m_pFormatCtx;
	AVPacket* m_pflush_pkt;
	////////////////////////Video Paras//////////////////////////////////////////////////////////////////////////////////

	int32_t m_iVideoStream; //<!视频流的序列
	AVCodecContext  *m_pVideoCodecCtx;
	AVCodecParameters* m_pVideoCodecPara;
	AVCodec *m_pVideoCodec;
	AVFrame  *m_pVideoFrame;
	AVFrame* m_pFrameYUV;
	int32_t m_iBufferSizeInBytes;

	struct SwsContext * m_pSws_ctx;



	SDL_Rect  m_rect;
	SDL_Event m_event;
	//int32_t m_iVideoFrameFinished;    //!< 当一帧处理完后的返回值

	double m_iFPS;    //<! FPS !>
	int32_t m_iFrameDelay; //<! frame delay !>
	VideoQueue* m_VideoQueue;
	//uint8_t* m_pVideoBufferForUnity;//<!储存解码视频的原始数据
	uint32_t m_iFilmWidth; //<! 影片的宽
	uint32_t m_iFilmHeight; //<!影片的高

	SDL_cond* m_pGPUMemCon;//唤醒线程条件量
	SDL_mutex* m_pGetFrameMutex;//<!获取渲染完成帧图像的互斥量
	SDL_mutex* m_pGpuMemMapMutex; //<!修改显存map的互斥量
	std::map<PictureTexture*, int32_t>* m_pGpuMemMap; //<!显存map ， 映射关系为<显存地址 ， 能否使用>

#if USE_SDL_VIDEO
	uint8_t* m_pSDLVideoBuffer;//<!储存解码视频的原始数据
	int64_t m_iMemcpyCostTime;
	SDL_Texture* m_pSDLTexture;
	SDL_Window* m_pScreen;
	SDL_Renderer* m_pRender;
#else
	uint8_t* m_pVideoBuffer;
	IDXGISwapChain* m_pD11SwapChain;
	ID3D11Device* m_pD11Device;
	ID3D11DeviceContext* m_pD11DeviceContext;
	D3D11_TEXTURE2D_DESC m_d311Texture2DDesc;
#endif
	int32_t m_iIsScreenScale;
	int32_t m_iRenderTargetWidth;
	int32_t m_iRenderTargetHeight;
	ID3D11Texture2D* m_RenderTargetLeft[RENDER_TARGET_TEXTURE_NUM];
	ID3D11Texture2D* m_RenderTargetRight[RENDER_TARGET_TEXTURE_NUM];

	uint32_t m_iLastFrameUploaded;
	HANDLE m_hSharedHandle;
	//ID3D11Texture2D* m_pTmpTexture;
	//ID3D11Resource* m_pTmpResource;
	//std::vector<ID3D11Texture2D*>* m_pFrameQueue;
	////////////////////////Audio Paras//////////////////////////////////////////////////////////////////////////////////
	//Buffer:
	//|-----------|-------------|
	//chunk-------pos---len-----|
	uint8_t  *audio_chunk;
	int32_t  audio_len;
	uint8_t  *audio_pos;

	SDL_AudioDeviceID m_CurrentAudioDeviceID;



	double audio_delay;   //<! 每两个音频之间的时间差 !>



	AudioParams m_OutAudioParams;
	AudioParams m_SrcAudioParams;

	//uint64_t out_channel_layout;
	//int32_t out_nb_samples;
	//AVSampleFormat out_sample_fmt;
	//int32_t out_sample_rate;
	//int32_t out_channels;
	//uint32_t out_buffer_size;
	//int32_t out_line_size;
	//int32_t out_bytes_per_sec;


	//uint64_t src_channel_layout;
	//int32_t src_channels;
	//int32_t src_sample_rate;
	//AVSampleFormat src_sample_fmt;


	//SDL_AudioSpec wanted_spec;

	int32_t m_iAudioStream;  //<! 音频流的序列 !>

	struct SwrContext *au_convert_ctx;
	AVFrame  *m_pAudioFrame;
	AVCodec *m_pAudioCodec;
	AVCodecParameters* m_pAudioCodecPara;
	AVCodecContext  *m_pAudioCodecCtx;
	//int32_t m_iAudioFrameFinished;
	AudioQueue* m_AudioQueue;

	enum AudioCBLockStatus m_eAudioCBStatus;
	//!<end

}FfVideoState;


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}


typedef enum AudioCBLockStatus{
	AUDIO_CB_UNLOCKED,
	AUDIO_CB_LOCKED
}AudioCBLockStatus;

typedef enum GpuMemFlag{
	GPU_MEM_DISABLE,
	GPU_MEM_ENABLE
}GpuMemFlag;


typedef enum Video3DMode{
	Video3DMode_Off = 0,
	Video3DMode_SideBySide = 1
}Video3DMode;

typedef enum PlayerType{
	ffmpeg = 1
}PlayerType;

typedef enum MovieType
{
	ONLINE,
	LOCAL
} MovieType;

typedef enum ExtClockStarted
{
	EXT_CLOCK_STOP,
	EXT_CLOCK_STARTED
} ExtClockStarted;

typedef enum HWAccel
{
	HWAccel_Off,
	HWAccel_On
} HWAccel;


typedef enum ReplayType
{
	MOVIE_REPLAY,
	MOVIE_NO_REPLAY
} ReplayType;

typedef enum PlayStatus
{
	STOP,
	PLAY,
	PAUSE
} PlayStatus;

typedef enum NetworkStatus
{
	NETWORK_NORMAL,
	NETWORK_CONNECTION_ERROR,
	NETWORK_INVALID_DATA
} NetworkStatus;

typedef enum NetworkBufferingStatus
{
	NETWORK_BUFFER_NORMAL,
	NETWORK_BUFFER_BUFFERING,
} NetworkBufferingStatus;

typedef enum SeekStatus
{
	SEEK_NOTHING,
	SEEK_LEFT,
	SEEK_RIGHT,
	SEEK_TIME
} SeekStatus;

typedef enum SynMethod
{
	SYN_MULTI,  //<! 通过时间戳和音频进行同步 !>
	SYN_FRAMERATE           //<! 通过帧率同步 !>
} SynMethod;

typedef enum IsFirstFrame
{
	FIRST_FRAME,
	NOT_FIRST_FRAME
} IsFirstFrame;

typedef enum BufferCleanFlag
{
	BUFFER_NO_CLEAN,
	BUFFER_CLEAN
} BufferCleanFlag;

typedef enum PlayProgressBarFlag
{
	REFRESH_PROGRESSBAR,
	NOT_REFRESH_PROGRESSBAR
} PlayProgressBarFlag;

typedef enum PlayFinishedFlag
{
	PLAY_NOT_FINISHED,
	PLAY_FINISHED
} PlayFinishedFlag;

typedef enum SeekSuccessFlag
{
	SEEK_SUCCESS,
	SEEK_FAIL
} SeekSuccessFlag;

typedef enum SeekInCacheFlag
{
	SEEK_NOT_IN_CACHE = 1,
	SEEK_IN_CACHE
} SeekInCacheFlag;

typedef enum CacheFinishedFlag
{
	CACHE_NOT_FINISHED,
	CACHE_FINISHED
} CacheFinishedFlag;

#define PRINT_LOG(...)  SDL_Log(__VA_ARGS__)


#define CHECK_PTR_RETURN_IF_NULL(x , log , returnValue)   \
if(nullptr == x){\
	PRINT_LOG(log);\
	return returnValue;\
}

#define RETURN_VALUE_SUCCESS 1
#define RETURN_VALUE_FAILED -1