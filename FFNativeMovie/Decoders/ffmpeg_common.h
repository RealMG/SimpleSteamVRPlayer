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

#define VideoBufferMaxSize MAX_GPU_BUFFERS//  //��Ƶ���������ֵ�����ڴ�ֵ ����������
#define VideoBufferMinSize 1//  //��Ƶ��������Сֵ��С�ڴ�ֵ����������

#define AudioBufferMaxSize 500//  //��Ƶ���������ֵ�����ڴ�ֵ ����������
#define AudioBufferMinSize 1//  //��Ƶ��������Сֵ��С�ڴ�ֵ����������

#define CacheBufferMaxSize 65534// FAT16�ļ�ϵͳÿ���ļ����������ļ�������FAT32��NTFS�����Ҫ�࣬���Կ��ܵ���СֵΪ׼�� 
#define CacheBufferMinSize 30//  


#define MAX_CACHE_SPACE_IN_KB 100<<11   //<! 200MB !> 
#define MIN_FILE_SIZE_IN_BYTES 4<<10 //<! 4KB !>


#define MIN_CACHE_FILE_COUNT 100  //<! �����ļ�С�ڴ�ֵʱ����ͣ���� !>
#define MIN_BUFFER_STATUS_FRAME_COUNT 500 //<! ������󣬿��Լ������ŵ������ļ��� !>

#define MAX_VIDEO_DIFFER_TIME 0.2 

#define MAX_VIDEO_AUDIO_DIFFER_TIME 0.3  //<! ����Ƶ����ͬ�������� !>
#define MIDDLE_VIDEO_AUDIO_DIFFER_TIME 0.15   //<! ����Ƶ����ͬ�������� !>
#define MIN_VIDEO_AUDIO_DIFFER_TIME 0.04   //<! ����Ƶ����ͬ�������� !>

#define MAX_VIDEO_DIFFER_ERROR 60   //<! ��Ƶʱ��������ֵ���������ֵ���л�ͬ����ʽ !>

#define DECODER_THREAD_COUNT 8

#define STREAM_TRACK_EMPTY -1

#define PACKET_THROW_COUNT_AFTER_SEEK 10

#define THREAD_CON_WAIT_TIMEOUT 500   //<! �̵߳ĵȴ���ʱʱ�� !>

#define DEFAULT_WINDOW_WIDTH 1024

#define DEFAULT_WINDOW_HEIGHT 768

#define SDL_THREAD_WAITTIMEOUT 100


/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 60

//�洢��Ƶ�Ķ���
typedef struct AudioQueueItem
{
	uint8_t *AudioData;//��Ƶ����
	int32_t Length;//��Ƶ����
	double Pts;//ʱ���
	AudioQueueItem *Next;//β��
}AudioQueueItem;

typedef struct AudioQueue
{
	AudioQueueItem *FirstItem;//����ͷ
	AudioQueueItem *LastItem;//����λ
	int32_t Length;//���г���

	SDL_mutex *audioMutex;//����ͬ�������߳�ͬʱ�������е� ������
	SDL_cond *audioCond;//�����߳�

}AudioQueue;




typedef struct _PictureTexture
{
	RenderTexture* rt;
#if USE_SDL_VIDEO
	uint8_t* raw_pixs;
#endif
}PictureTexture;



//�洢��Ƶ�Ķ���
typedef struct VideoQueueItem
{
	PictureTexture* Texture;//��Ƶ����
	int32_t Width;//��ƵͼƬ�Ŀ��
	int32_t Height;//��ƵͼƬ�ĸ߶�
	int32_t Length;//��Ƶ����
	double Pts;//ʱ���
	VideoQueueItem *Next;//β��
}VideoQueueItem;

typedef struct VideoQueue
{
	VideoQueueItem *FirstItem;//����ͷ
	VideoQueueItem *LastItem;//����λ
	int32_t Length;//���г���
	double BufferPts;//�����pts

	SDL_mutex *videoMutex;//����ͬ�������߳�ͬʱ�������е� ������
	SDL_cond *videoCond;//�����߳�

}VideoQueue;


typedef struct FirstFrame
{
	enum IsFirstFrame isFirstFrame;  //<! �жϵ�һ֡��m3u8�ļ�����һ֡ȡ����ʱ������ܲ���0�������ʾ��seek��Ҫ��ƫ�� !>
	double FirstFramePts;    //<! ��һ֡��ʱ�䣬���߶���Ϊ��Ҫƫ�Ƶ�ʱ�� !
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

	double m_iSeekTime;  //<! ��Ҫ��ת����ʱ��
	int32_t m_iCurrentVolume;//<! ��ǰ������ 

	//int32_t m_iPacketThrowCount;    //<! packet����������seek����Ҫ������֡��������֤ʱ����ȷ !>


	enum CacheFinishedFlag m_iCacheFinishedFlag;//<! �Ƿ��ѻ��嵽��� !>
	enum PlayStatus m_iPlayingFlag;// <! ����״̬enum PlayStatusFlag;
	enum BufferCleanFlag m_iBufferCleanFlag;//<!�Ƿ���Ҫ��ջ��档��seekʱ��Ҫ��ѭ������ʱ����Ҫ���
	enum ReplayType m_iMovieReplayFlag;//<!�Ƿ���Ҫ�ظ�����                                                                                          //<! �Զ��ظ����ţ���ǰ��Ч !>
	enum SeekSuccessFlag m_iIsSeekSuccessFlag;//<!seekʱ�Ƿ�ɹ��������ɹ�һ�Σ����ظ�seek ��      //<! �Զ��ظ����ţ���ǰ��Ч !>
	enum NetworkStatus m_iNetworkStatusFlag;//<! ����״̬����ֵ !>
	enum NetworkBufferingStatus m_iNetworkBufferingStatusFlag;//<! �����Ƿ����ڻ��巵��ֵ !>
	enum PlayFinishedFlag m_iPlayFinishedFlag;//<! �Ƿ񲥷ŵ���� !>
	enum PlayProgressBarFlag m_iPlayProgressBarRefreshFlag;//<!�Ƿ���Ҫ���²��Ž�����������ͣ״̬�����������תʱ������Ҫ����      //<! �Զ��ظ����ţ���ǰ��Ч !>
	enum SeekInCacheFlag m_iSeekInCacheFlag;  //<! �ڻ�������ת��ֻ��Online��������Ч
	enum SynMethod m_iSynMethod;//<! ͬ����ʽ
	enum SynMethod m_iFirstSynMethodSelected;//<! �����һ��ѡ���ͬ����ʽ
	enum SeekStatus m_iSeekFlag;  //<! seek ״̬ !>

	//FirstFrame m_sFirstAudioFrame;
	//FirstFrame m_sFirstVideoFrame;
	FirstFrame m_sFirstFrame;   //<! ������Ƶ���õ�һ����ʱ�䵱��ƫ��ʱ�� !>

	//ExtClock m_sExtClock;   //<! �ⲿʱ�ӣ�Ч������ͣ�� !>

	//double m_dLastDisplayPosition;
	double m_dCurrentPosition;
	double m_dCurrentVideoPts; //<! ��ǰ��Ƶʱ���
	double m_dCurrentAudioPts; //<! ��ǰ��Ƶʱ���
	int64_t m_dFilmDuration; //<! ӰƬ����
	double m_dFilmDurationInSecs; //<! ת��Ϊ����ӰƬ����

	double m_dCurrentDecodeVideoPts; //<! �������Ƶʱ��� !>
	double m_dCurrentDecodeAudioPts; //<! �������Ƶʱ��� !>

	double m_dCurrentCacheVideoPts; //<! ������Ƶʱ���
	double m_dCurrentCacheAudioPts; //<! ������Ƶʱ���
	double m_dCurrentCachePosition;//<! ���嵽��λ�� !>
	uint64_t m_ui64CurrentCacheLength; //<! ������еĳ��� !>

	AVFormatContext *m_pFormatCtx;
	AVPacket* m_pflush_pkt;
	////////////////////////Video Paras//////////////////////////////////////////////////////////////////////////////////

	int32_t m_iVideoStream; //<!��Ƶ��������
	AVCodecContext  *m_pVideoCodecCtx;
	AVCodecParameters* m_pVideoCodecPara;
	AVCodec *m_pVideoCodec;
	AVFrame  *m_pVideoFrame;
	AVFrame* m_pFrameYUV;
	int32_t m_iBufferSizeInBytes;

	struct SwsContext * m_pSws_ctx;



	SDL_Rect  m_rect;
	SDL_Event m_event;
	//int32_t m_iVideoFrameFinished;    //!< ��һ֡�������ķ���ֵ

	double m_iFPS;    //<! FPS !>
	int32_t m_iFrameDelay; //<! frame delay !>
	VideoQueue* m_VideoQueue;
	//uint8_t* m_pVideoBufferForUnity;//<!���������Ƶ��ԭʼ����
	uint32_t m_iFilmWidth; //<! ӰƬ�Ŀ�
	uint32_t m_iFilmHeight; //<!ӰƬ�ĸ�

	SDL_cond* m_pGPUMemCon;//�����߳�������
	SDL_mutex* m_pGetFrameMutex;//<!��ȡ��Ⱦ���֡ͼ��Ļ�����
	SDL_mutex* m_pGpuMemMapMutex; //<!�޸��Դ�map�Ļ�����
	std::map<PictureTexture*, int32_t>* m_pGpuMemMap; //<!�Դ�map �� ӳ���ϵΪ<�Դ��ַ �� �ܷ�ʹ��>

#if USE_SDL_VIDEO
	uint8_t* m_pSDLVideoBuffer;//<!���������Ƶ��ԭʼ����
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



	double audio_delay;   //<! ÿ������Ƶ֮���ʱ��� !>



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

	int32_t m_iAudioStream;  //<! ��Ƶ�������� !>

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
	SYN_MULTI,  //<! ͨ��ʱ�������Ƶ����ͬ�� !>
	SYN_FRAMERATE           //<! ͨ��֡��ͬ�� !>
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