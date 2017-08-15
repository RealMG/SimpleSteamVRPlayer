/*!
 * \file ffmpeg.cpp
 *
 * \author Nico
 * \date
 *
 *
 */


#include "ffmpeg.h"
#include "stdio.h"

Ffmpeg::Ffmpeg() :
	m_iWndWidth(DEFAULT_WINDOW_WIDTH),
	m_iWndHeight(DEFAULT_WINDOW_HEIGHT),
	m_hHwnd(0)
{
}

Ffmpeg::~Ffmpeg()
{

}


int32_t Ffmpeg::Play()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	return m_pVideoState->m_iPlayingFlag = PlayStatus::PLAY;
}

uint32_t Ffmpeg::SeekTo(uint64_t position, int32_t isBufferClean, int32_t isPlayProgressBarRefresh)
{

	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	m_pVideoState->m_iSeekTime = (double)position / 10000 / 1000;


	if ((m_pVideoState->m_iSeekTime < m_pVideoState->m_dCurrentCachePosition) && (m_pVideoState->m_iSeekTime > m_pVideoState->m_dCurrentPosition))
	{
		m_pVideoState->m_iSeekInCacheFlag = SeekInCacheFlag::SEEK_IN_CACHE;  //<! 在缓存中跳转，仅对在线播放器有效 !>
	}
	else
	{
		//m_pVideoState->m_iNetworkStatusFlag = NetworkStatus::NETWORK_BUFFERING;  //<! 在缓存外跳转时清空缓存，必然会先缓冲 !>
		m_pVideoState->m_iSeekInCacheFlag = SeekInCacheFlag::SEEK_NOT_IN_CACHE;
	}

	m_pVideoState->m_iSeekFlag = SeekStatus::SEEK_TIME;
	m_pVideoState->m_iBufferCleanFlag = (enum BufferCleanFlag)isBufferClean;
	m_pVideoState->m_iPlayProgressBarRefreshFlag = (enum PlayProgressBarFlag)isPlayProgressBarRefresh;
	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::Stop()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	if (nullptr != m_pVideoState)
	{

		m_pVideoState->m_iPlayingFlag = PlayStatus::STOP;

		if (nullptr != this->m_pReadThread)
		{
			SDL_WaitThread(this->m_pReadThread, NULL);//<!等待所有线程退出后再进行清理
		}

	}

	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::InitDecoder()
{

	avcodec_register_all();//注册所有的ffmpeg decoder
	av_register_all();//注册所有的格式
	avformat_network_init();//初始化网络

	if (av_lockmgr_register(&Ffmpeg::LockMgr))
	{
		PRINT_LOG("Could not initialize lock manager!\n");
		return RETURN_VALUE_FAILED;
	}//注册ffmpeg的lockmgr回调函数。用以锁定avcodec_open/close，确保其为原子操作。


	m_pVideoState->m_pFormatCtx = avformat_alloc_context();
	m_pVideoState->m_pFormatCtx->interrupt_callback.callback = &Ffmpeg::InterruptCallBack;
	m_pVideoState->m_pFormatCtx->interrupt_callback.opaque = m_pVideoState;

	AVDictionary* options = nullptr;
	av_dict_set(&options, "stimeout", "10", 0);  //设置超时断开连接时间


	int err = avformat_open_input(&m_pVideoState->m_pFormatCtx, m_sFilename.data(), NULL, &options);
	if (err != 0)
	{
		char err_str[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(err_str, AV_ERROR_MAX_STRING_SIZE, err);
		PRINT_LOG("Could not open file!\n");
		return RETURN_VALUE_FAILED; // Couldn't open file
	}//<！根据url打开文件


	if (avformat_find_stream_info(m_pVideoState->m_pFormatCtx, NULL) < 0)
	{
		PRINT_LOG("Couldn't find stream information  !\n");
		return RETURN_VALUE_FAILED; // Couldn't find stream information 
	}  //!< 查询流媒体信息

	//<！Dump information about file onto standard error  
	av_dump_format(m_pVideoState->m_pFormatCtx, 0, m_sFilename.data(), 0);

	//<！Find the first video stream  

	m_pVideoState->m_iVideoStream = STREAM_TRACK_EMPTY;
	m_pVideoState->m_iAudioStream = STREAM_TRACK_EMPTY;

	for (uint32_t ii = 0; ii < m_pVideoState->m_pFormatCtx->nb_streams; ii++)
	{
		if (m_pVideoState->m_pFormatCtx->streams[ii]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_pVideoState->m_iVideoStream = ii;
		}
		else if (m_pVideoState->m_pFormatCtx->streams[ii]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			m_pVideoState->m_iAudioStream = ii;
		}
	}

	m_pVideoState->m_pflush_pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
	av_init_packet(m_pVideoState->m_pflush_pkt);

	if (m_pVideoState->m_pflush_pkt == NULL)
	{
		PRINT_LOG("Could not create m_pflush_pkt!\n");
		return RETURN_VALUE_FAILED;
	}


	CalcFilmInfos();


	if (InitVideoQueue(&m_pVideoState->m_VideoQueue) != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Could not init video queue!\n");
		return RETURN_VALUE_FAILED;
	}

	if (InitAudioQueue(&m_pVideoState->m_AudioQueue) != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Could not init audio queue!\n");
		return RETURN_VALUE_FAILED;
	}

	if (m_pVideoState->m_iVideoStream != STREAM_TRACK_EMPTY)  //<! 当有视频时创建，否则不创建 !>
	{
		if (InitDecoderVideo() != RETURN_VALUE_SUCCESS)
		{
			PRINT_LOG("Could not init video decoder!\n");
			return RETURN_VALUE_FAILED;
		}
	}

	if (m_pVideoState->m_iAudioStream != STREAM_TRACK_EMPTY)  //<! 当有音频时创建，否则不创建 !>
	{
		if (InitDecoderAudio() != RETURN_VALUE_SUCCESS)
		{
			PRINT_LOG("Could not init audio decoder!\n");
			return RETURN_VALUE_FAILED;
		}
	}

	PRINT_LOG("Init Decoder ok \n");

	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::ReleaseDecoder()
{



	if (nullptr != m_pVideoState->m_pflush_pkt)
	{
		av_free(m_pVideoState->m_pflush_pkt);
		m_pVideoState->m_pflush_pkt = nullptr;
	}
	if (nullptr != m_pVideoState->m_pAudioFrame)
	{
		av_free(m_pVideoState->m_pAudioFrame);
		m_pVideoState->m_pAudioFrame = nullptr;
	}

	if (nullptr != m_pVideoState->m_pSws_ctx)
	{
		sws_freeContext(m_pVideoState->m_pSws_ctx);
		m_pVideoState->m_pSws_ctx = nullptr;
	}
	if (nullptr != m_pVideoState->au_convert_ctx)
	{
		swr_free(&m_pVideoState->au_convert_ctx);
		m_pVideoState->au_convert_ctx = nullptr;
	}

	if (nullptr != m_pVideoState->m_pAudioCodecCtx)
	{
		avcodec_close(m_pVideoState->m_pAudioCodecCtx);
		m_pVideoState->m_pAudioCodecCtx = nullptr;
	}
	if (nullptr != m_pVideoState->m_pVideoCodecCtx)
	{
		avcodec_close(m_pVideoState->m_pVideoCodecCtx);
		m_pVideoState->m_pAudioCodecCtx = nullptr;
	}

	if (nullptr != m_pVideoState->m_pFrameYUV)
	{
		av_free(m_pVideoState->m_pFrameYUV);
		m_pVideoState->m_pFrameYUV = nullptr;
	}
	if (nullptr != m_pVideoState->m_pVideoFrame)
	{
		av_free(m_pVideoState->m_pVideoFrame);
		m_pVideoState->m_pVideoFrame = nullptr;
	}

	if (nullptr != m_pVideoState->m_VideoQueue)
	{
		ClearVideoQueue(m_pVideoState->m_VideoQueue);
		ReleaseVideoQueue(&m_pVideoState->m_VideoQueue);
	}

	if (nullptr != m_pVideoState->m_AudioQueue)
	{
		ClearAudioQueue(m_pVideoState->m_AudioQueue);
		ReleaseAudioQueue(&m_pVideoState->m_AudioQueue);
	}

	if (nullptr != m_pVideoState->m_pFormatCtx)
	{
		avformat_close_input(&m_pVideoState->m_pFormatCtx);
	}
	avformat_network_deinit();

	return RETURN_VALUE_SUCCESS;
}



void Ffmpeg::CalcFilmInfos()
{
	m_pVideoState->m_dFilmDuration = (m_pVideoState->m_pFormatCtx->duration);
	m_pVideoState->m_dFilmDurationInSecs = ceil((m_pVideoState->m_pFormatCtx->duration) / AV_TIME_BASE);
}


int32_t Ffmpeg::Open(std::string fileName)
{

	SetFilePath(fileName);//<! For UnityMovie ;

	m_pVideoState = (FfVideoState*)av_mallocz(sizeof(FfVideoState));
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null\n", RETURN_VALUE_FAILED);

	m_pVideoState->m_iCurrentVolume = 70;  //<! 
	m_pVideoState->m_iPlayingFlag = PlayStatus::PAUSE;

	if (InitDecoder() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init Decoder failed\n");
		goto ERR;
	}

	if (InitSDL() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init SDL failed\n");
		goto ERR;
	}

	if (InitPicoHMD() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init Pico HMD failed\n");
		goto ERR;
	}

	if (InitD3d11() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init D3D failed\n");
		goto ERR;
	}


	if (InitGPUBuffers() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init GPU Buffers failed\n");
		goto ERR;
	}

	if (InitThread() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init threads failed\n");
		goto ERR;
	}





	ChooseSynMethod();


	return RETURN_VALUE_SUCCESS;

ERR:
	ReleaseSources();
	return RETURN_VALUE_FAILED;
}

int32_t Ffmpeg::ChooseSynMethod()
{
	if (STREAM_TRACK_EMPTY != m_pVideoState->m_iAudioStream)
	{
		m_pVideoState->m_iSynMethod = SynMethod::SYN_MULTI;
		if (m_pVideoState->m_iFPS != 0)
		{
			m_pVideoState->m_iFrameDelay = 1000 / (double)m_pVideoState->m_iFPS;
		}

	}
	else
	{
		m_pVideoState->m_iSynMethod = SynMethod::SYN_FRAMERATE;
		m_pVideoState->m_iFrameDelay = (m_pVideoState->m_iFPS == 0 ? (double)1000 / (double)30 : (double)1000 / (double)m_pVideoState->m_iFPS);  //<! 若获取不到任何播放时间信息，选取默认30fps !>
	}

	m_pVideoState->m_iFirstSynMethodSelected = m_pVideoState->m_iSynMethod;   //<! 存储第一次选择的方式，用来恢复同步方式 !>

	return RETURN_VALUE_SUCCESS;
}


int32_t Ffmpeg::ReleaseSources()
{
	ReleaseDecoder();
	ReleaseGPUBuffers();
	ReleaseD3D11();
	ReleaseSDL();
	ReleasePicoHMD();
	if (nullptr != m_pVideoState)
	{
		av_free(m_pVideoState);
		m_pVideoState = NULL;
	}

	return RETURN_VALUE_SUCCESS;
}

int64_t Ffmpeg::GetFilmDuration()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	int64_t _duration = ceil(((double)m_pVideoState->m_dFilmDuration) / AV_TIME_BASE) * 1000 * 10000;  //<! In Ticks !>
	return _duration;
}


int32_t Ffmpeg::InitSDL()
{

	if (SetEnvironmentVariableW(L"SDL_AUDIODRIVER", L"directsound") == 0)//<! 使用DirectSound而不是默认的XAudio2，因为XAudio2拔插头戴后，无法回调AudioCallBack() !>
	{
		PRINT_LOG("can't set env to directsound, use xaudio2.\n");
	}

	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		PRINT_LOG("Could not initialize SDL! \n");
		return RETURN_VALUE_FAILED;
	}//初始化SDL

	if (m_pVideoState->m_iVideoStream != STREAM_TRACK_EMPTY)  //<! 当有视频时创建，否则不创建 !>
	{
		//PRINT_LOG("init sdl video");
		if (InitSDLVideo() != RETURN_VALUE_SUCCESS)
		{
			PRINT_LOG("Could not initialize SDL Video \n");
			return RETURN_VALUE_FAILED;
		}
	}

	//if (m_pVideoState->m_iAudioStream != STREAM_TRACK_EMPTY)  //<! 当有音频时创建，否则不创建 , 音频问题，移到解码线程中动态创建!>
	//{
	//	if (InitSDLAudio() != RETURN_VALUE_SUCCESS)
	//	{
	//		PRINT_LOG("Could not initialize SDL Audio \n");
	//		return RETURN_VALUE_FAILED;
	//	}
	//}
	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::InitSDLVideo()
{


	m_pVideoState->m_pGpuMemMapMutex = SDL_CreateMutex();
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pGpuMemMapMutex, "m_pGpuMemMapMutex is null! \n", RETURN_VALUE_FAILED);

#if USE_SDL_VIDEO

	m_pVideoState->m_pGetFrameMutex = SDL_CreateMutex();
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pGetFrameMutex, "m_pGetFrameMutex is null! \n", RETURN_VALUE_FAILED);
	//<!从传递进来的HWND窗口句柄创建一个用于显示的Screen

	//m_pVideoState->m_pScreen = SDL_CreateWindow("PicoPlayer",
	//	SDL_WINDOWPOS_CENTERED,
	//	SDL_WINDOWPOS_CENTERED,
	//	m_iWndWidth,
	//	m_iWndHeight,
	//	SDL_WINDOW_SHOWN);
	//HWND hnd =  FindWindow(L"QtGuiApplication1", L"11111");
	//PRINT_LOG("%ld", hnd);
	m_pVideoState->m_pScreen = SDL_CreateWindowFrom((void *)m_hHwnd);
	if (m_pVideoState->m_pScreen == NULL)
	{
		PRINT_LOG("Could not create m_pScreen  !\n");
		//LW_LOG(L"Could not create m_pScreen !\n");
		return RETURN_VALUE_FAILED;
	}
	PRINT_LOG("Create m_pScreen %ld, hwnd %ld", m_pVideoState->m_pScreen, m_hHwnd);

	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	//SDL_RaiseWindow(m_pVideoState->m_pScreen);
	m_pVideoState->m_rect.w = m_pVideoState->m_iRenderTargetWidth;  //<!-- set source texture size according to PicoPlugin render target size, also texture size after decoder -->
	m_pVideoState->m_rect.h = m_pVideoState->m_iRenderTargetHeight;

	//m_pVideoState->m_rect.w = m_iWndWidth;
	//m_pVideoState->m_rect.h = m_iWndHeight;

	//m_pVideoState->m_rect.w = m_pVideoState->m_pVideoCodecCtx->width;
	//m_pVideoState->m_rect.h = m_pVideoState->m_pVideoCodecCtx->height;
	m_pVideoState->m_rect.x = 0;
	m_pVideoState->m_rect.y = 0;

	//!<创建一个渲染器
	m_pVideoState->m_pRender = SDL_CreateRenderer(m_pVideoState->m_pScreen, -1, 0);
	if (m_pVideoState->m_pRender == NULL)
	{
		PRINT_LOG("Could not create m_pRender  !\n");
		//LW_LOG(L"Could not create m_pRender !\n");
		return RETURN_VALUE_FAILED;
	}

	m_pVideoState->m_pSDLTexture = SDL_CreateTexture(m_pVideoState->m_pRender,
		SDL_FORMAT_CURRENT_USED, SDL_TEXTUREACCESS_STREAMING,
		m_pVideoState->m_rect.w,
		m_pVideoState->m_rect.h
	);



	m_pVideoState->m_pSDLVideoBuffer = (uint8_t*)av_mallocz(m_pVideoState->m_iBufferSizeInBytes);
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pSDLVideoBuffer, "m_pSDLVideoBuffer is null! \n", RETURN_VALUE_FAILED);

	//m_pVideoState->m_iWidgetWidth = m_iWndWidth;//<!此处设置用于显示的外部窗体大小，用于resize等操作。
	//m_pVideoState->m_iWidgetHeight = m_iWndHeight;
#endif

	return RETURN_VALUE_SUCCESS;

}
int32_t Ffmpeg::InitSDLAudio(uint64_t channellayout, int32_t channels, int32_t samplerate, int32_t samples)
{
	/*m_pVideoState->wanted_spec.freq = m_pVideoState->m_pAudioCodecCtx->sample_rate;
	m_pVideoState->wanted_spec.format = AUDIO_S16SYS;
	m_pVideoState->wanted_spec.channels = m_pVideoState->out_channels;
	m_pVideoState->wanted_spec.samples = m_pVideoState->out_nb_samples;
	m_pVideoState->wanted_spec.callback = &Ffmpeg::SDL_AudioCallBack;
	m_pVideoState->wanted_spec.userdata = this;

	if (SDL_OpenAudio(&m_pVideoState->wanted_spec, NULL) < 0) {
		PRINT_LOG("can't open audio.\n");
		return RETURN_VALUE_FAILED;
	}

	PRINT_LOG("SDL Audio open ok \n");*/

	if (SDL_AUDIO_STOPPED != SDL_GetAudioStatus())
	{
		SDL_PauseAudio(1);
		SDL_CloseAudio();
	}



	uint64_t wanted_channel_layout = channellayout;
	int32_t wanted_nb_channels = channels;
	int32_t wanted_sample_rate = samplerate;


	SDL_AudioSpec wanted_spec, spec;
	const char *env;
	static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
	static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
	int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

	env = SDL_getenv("SDL_AUDIO_CHANNELS");
	if (env) {
		wanted_nb_channels = atoi(env);
		wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
	}
	if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
		wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
	}
	wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
	wanted_spec.channels = wanted_nb_channels;
	wanted_spec.freq = wanted_sample_rate;
	if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
		PRINT_LOG("Invalid sample rate or channel count!\n");
		return RETURN_VALUE_FAILED;
	}
	while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
		next_sample_rate_idx--;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.silence = 0;
	//wanted_spec.samples = m_pVideoState->out_nb_samples;
	//wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
																	   //<!-- Debug -->	
	wanted_spec.samples = samples;//FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));

	wanted_spec.callback = nullptr;//&Ffmpeg::SDL_AudioCallBack;
	wanted_spec.userdata = this;



	//do {

	//	SDL_AudioDeviceID id = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);

	//	if (NULL == id)
	//	{
	//		PRINT_LOG("SDL_OpenAudio (%d channels, %d Hz): %s\n",
	//			wanted_spec.channels, wanted_spec.freq, SDL_GetError());
	//		wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
	//		if (!wanted_spec.channels) {
	//			wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
	//			wanted_spec.channels = wanted_nb_channels;
	//			if (!wanted_spec.freq) {
	//				av_log(NULL, AV_LOG_ERROR,
	//					"No more combinations to try, audio open failed\n");
	//				return RETURN_VALUE_FAILED;
	//			}
	//		}
	//		wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
	//	}
	//	else
	//	{
	//		m_pVideoState->m_CurrentAudioDeviceID = id;
	//	}	
	//
	//} while (NULL == m_pVideoState->m_CurrentAudioDeviceID);


	while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		PRINT_LOG("SDL_OpenAudio (%d channels, %d Hz): %s\n",
			wanted_spec.channels, wanted_spec.freq, SDL_GetError());
		wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
		if (!wanted_spec.channels) {
			wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
			wanted_spec.channels = wanted_nb_channels;
			if (!wanted_spec.freq) {
				PRINT_LOG(
					"No more combinations to try, audio open failed\n");
				return RETURN_VALUE_FAILED;
			}
		}
		wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
	}


	if (spec.format != AUDIO_S16SYS) {
		PRINT_LOG(
			"SDL advised audio format %d is not supported!\n", spec.format);
		return RETURN_VALUE_FAILED;
	}
	if (spec.channels != wanted_spec.channels) {
		wanted_channel_layout = av_get_default_channel_layout(spec.channels);
		if (!wanted_channel_layout) {
			PRINT_LOG(
				"SDL advised channel count %d is not supported!\n", spec.channels);
			return RETURN_VALUE_FAILED;
		}
	}


	////Out Audio Param
	//int32_t _defaultChannels = av_get_channel_layout_nb_channels(m_pVideoState->m_pAudioCodecCtx->channel_layout);
	//m_pVideoState->m_OutAudioParams.channels = _defaultChannels > 2 ? 2 : _defaultChannels;
	//m_pVideoState->m_OutAudioParams.channel_layout = av_get_default_channel_layout(m_pVideoState->m_OutAudioParams.channels); //<! 任何时候都需要通过声道数查找布局方式 !>
	////<! SDL最大支持2个声道，所以ffmpeg在转码的时候应该将多声道的转化为双声道 !>

	//m_pVideoState->m_OutAudioParams.freq = m_pVideoState->m_pAudioCodecCtx->sample_rate;

	////m_pVideoState->out_nb_samples = m_pVideoState->m_pAudioCodecCtx->frame_size > 0 ? m_pVideoState->m_pAudioCodecCtx->frame_size:FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(m_pVideoState->out_sample_rate / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
	//
	////<! frame_size 可能为0 ， 后面的计算方式是从ffplay中拿来的，将SDL_AUDIO_MAX_CALLBACKS_PER_SEC修改为60后，测试视频正常，为什么这样计算未知。 !>
	//m_pVideoState->m_OutAudioParams.fmt = AV_SAMPLE_FMT_S16;
	////Out Buffer Size
	//int out_buffer_size = av_samples_get_buffer_size(
	//	NULL,
	//	m_pVideoState->m_OutAudioParams.channels,
	//	m_pVideoState->m_OutAudioParams.freq,
	//	m_pVideoState->m_OutAudioParams.fmt,
	//	0
	//	);
	//m_pVideoState->m_OutAudioParams.bytes_per_sec = m_pVideoState->m_pAudioCodecCtx->sample_rate * m_pVideoState->m_OutAudioParams.channels * 2;  //<! 计算音频每秒字节数 !>
	////m_pVideoState->audio_delay = (double)(m_pVideoState->out_buffer_size) / (double)(m_pVideoState->out_bytes_per_sec);





	m_pVideoState->m_OutAudioParams.fmt = AV_SAMPLE_FMT_S16;
	m_pVideoState->m_OutAudioParams.freq = spec.freq;
	m_pVideoState->m_OutAudioParams.channel_layout = wanted_channel_layout;
	m_pVideoState->m_OutAudioParams.channels = spec.channels;
	//m_pVideoState->m_OutAudioParams.frame_size = av_samples_get_buffer_size(NULL, m_pVideoState->m_OutAudioParams.channels, 1, m_pVideoState->m_OutAudioParams.fmt, 1);
	m_pVideoState->m_OutAudioParams.frame_size = av_samples_get_buffer_size(NULL, m_pVideoState->m_OutAudioParams.channels, spec.samples, m_pVideoState->m_OutAudioParams.fmt, 0);
	//<!-- Debug -->	
	m_pVideoState->m_OutAudioParams.bytes_per_sec = av_samples_get_buffer_size(
		NULL,
		m_pVideoState->m_OutAudioParams.channels,
		m_pVideoState->m_OutAudioParams.freq,
		m_pVideoState->m_OutAudioParams.fmt,
		1
	);
	if (m_pVideoState->m_OutAudioParams.bytes_per_sec <= 0 || m_pVideoState->m_OutAudioParams.frame_size <= 0) {
		PRINT_LOG("av_samples_get_buffer_size failed\n");
		return RETURN_VALUE_FAILED;
	}
	m_pVideoState->m_OutAudioParams.nb_samples = spec.samples;


	m_pVideoState->audio_delay = (double)(m_pVideoState->m_OutAudioParams.nb_samples) / (double)(m_pVideoState->m_OutAudioParams.freq);


	m_pVideoState->m_SrcAudioParams = m_pVideoState->m_OutAudioParams;

	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::ReleaseSDL()
{
	SDL_CloseAudio();

#if USE_SDL_VIDEO
	SDL_DestroyRenderer(m_pVideoState->m_pRender);
	m_pVideoState->m_pRender = NULL;

	if (nullptr != m_pVideoState->m_pSDLVideoBuffer)
	{
		av_free(m_pVideoState->m_pSDLVideoBuffer);
		m_pVideoState->m_pSDLVideoBuffer = nullptr;
	}

	if (nullptr != m_pVideoState->m_pGetFrameMutex)
	{
		SDL_DestroyMutex(m_pVideoState->m_pGetFrameMutex);
	}
#endif
	if (nullptr != m_pVideoState->m_pGpuMemMapMutex)
	{
		SDL_DestroyMutex(m_pVideoState->m_pGpuMemMapMutex);
	}


	SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
	//atexit(SDL_Quit);//<! 从SDL_QuitSubSystem()修改为SDL_Quit()，因为丢失设备后，前者会阻塞无法退出 !>

	return RETURN_VALUE_SUCCESS;
}


int32_t Ffmpeg::InitDecoderVideo()
{

	//<！Get a pointer to the codec context for the video stream  
	m_pVideoState->m_pVideoCodecPara = m_pVideoState->m_pFormatCtx->streams[m_pVideoState->m_iVideoStream]->codecpar;

	//<!  获取帧率 !>
	m_pVideoState->m_iFPS = av_q2d(m_pVideoState->m_pFormatCtx->streams[m_pVideoState->m_iVideoStream]->avg_frame_rate);


	//<！Find the decoder for the video stream  
	m_pVideoState->m_pVideoCodec = avcodec_find_decoder(m_pVideoState->m_pVideoCodecPara->codec_id);
	if (m_pVideoState->m_pVideoCodec == NULL)
	{
		PRINT_LOG("Unsupported codec!\n");
		return RETURN_VALUE_FAILED; // Codec not found  
	}//<！没有支持的解码器


	m_pVideoState->m_pVideoCodecCtx = avcodec_alloc_context3(m_pVideoState->m_pVideoCodec);
	if (m_pVideoState->m_pVideoCodecCtx == NULL)
	{
		PRINT_LOG("m_pCodecCtx NULL!\n");
		return RETURN_VALUE_FAILED; //<！m_pCodecCtx NULL  
	}//<！创建一个AVCodecContext

	if (avcodec_parameters_to_context(m_pVideoState->m_pVideoCodecCtx, m_pVideoState->m_pVideoCodecPara) < 0)
	{
		PRINT_LOG("Could convert parameters  !\n");
		return RETURN_VALUE_FAILED;
	}//<！根据AVCodecContext创建一个AVCodecParameters


	m_pVideoState->m_pVideoCodecCtx->thread_count = DECODER_THREAD_COUNT;
	//m_pVideoState->m_pVideoCodecCtx->thread_type = FF_THREAD_SLICE;

	if (avcodec_open2(m_pVideoState->m_pVideoCodecCtx, m_pVideoState->m_pVideoCodec, /*&m_pVideoState->m_pOptionsDict*/NULL) < 0)
	{
		PRINT_LOG("Could not open codec  !\n");
		return RETURN_VALUE_FAILED; // Could not open codec  
	} //<！打开解码器

	m_pVideoState->m_iFilmHeight = m_pVideoState->m_pVideoCodecCtx->height;
	m_pVideoState->m_iFilmWidth = m_pVideoState->m_pVideoCodecCtx->width;

	/*
		m_pVideoState->m_iRenderTargetWidth = m_pVideoState->m_iFilmWidth;
		m_pVideoState->m_iRenderTargetHeight = m_pVideoState->m_iFilmHeight;*/

		//<！Allocate video frame  
	m_pVideoState->m_pVideoFrame = av_frame_alloc();
	if (m_pVideoState->m_pVideoFrame == NULL)
	{
		PRINT_LOG("m_pFrame is NULL  !\n");
		return RETURN_VALUE_FAILED;
	}

	//<！分配一个转码Frame
	m_pVideoState->m_pFrameYUV = av_frame_alloc();
	if (m_pVideoState->m_pFrameYUV == NULL)
	{
		PRINT_LOG("m_pFrameYUV is NULL!\n");
		return RETURN_VALUE_FAILED;
	}


	m_pVideoState->m_iBufferSizeInBytes = av_image_get_buffer_size(
		AV_FORMAT_CURRENT_USED,
		m_pVideoState->m_iFilmWidth,
		m_pVideoState->m_iFilmHeight,
		1   //<! no alignment
	);


#if !USE_SDL_VIDEO

	m_pVideoState->m_pVideoBuffer = (uint8_t *)av_mallocz((m_pVideoState->m_iBufferSizeInBytes));
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pVideoBuffer, "VideoBufferForUnity null ptr! \n", RETURN_VALUE_FAILED);

	av_image_fill_arrays(
		m_pVideoState->m_pFrameYUV->data,
		m_pVideoState->m_pFrameYUV->linesize,
		m_pVideoState->m_pVideoBuffer,
		AV_FORMAT_CURRENT_USED,
		m_pVideoState->m_iFilmWidth,
		m_pVideoState->m_iFilmHeight,
		1
	);

#endif

	PRINT_LOG("Init Decoder video successfully\n");

	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::InitDecoderAudio()
{
	//<！Get a pointer to the codec context for the video stream  
	m_pVideoState->m_pAudioCodecPara = m_pVideoState->m_pFormatCtx->streams[m_pVideoState->m_iAudioStream]->codecpar;

	//<！Find the decoder for the Audio stream  
	m_pVideoState->m_pAudioCodec = avcodec_find_decoder(m_pVideoState->m_pAudioCodecPara->codec_id);
	if (m_pVideoState->m_pAudioCodec == NULL)
	{
		PRINT_LOG("Unsupported codec!\n");
		return RETURN_VALUE_FAILED; // Codec not found  
	}//<！没有支持的解码器

	m_pVideoState->m_pAudioCodecCtx = avcodec_alloc_context3(m_pVideoState->m_pAudioCodec);
	if (m_pVideoState->m_pAudioCodecCtx == NULL)
	{
		PRINT_LOG("m_pCodecCtx NULL!\n");
		return RETURN_VALUE_FAILED; //<！m_pCodecCtx NULL  
	}//<！创建一个AVCodecContext

	if (avcodec_parameters_to_context(m_pVideoState->m_pAudioCodecCtx, m_pVideoState->m_pAudioCodecPara) < 0)
	{
		PRINT_LOG("Could convert parameters  !\n");
		return RETURN_VALUE_FAILED;
	}//<！根据AVCodecContext创建一个AVCodecParameters


	if (avcodec_open2(m_pVideoState->m_pAudioCodecCtx, m_pVideoState->m_pAudioCodec,/* &m_pVideoState->m_pOptionsDict*/NULL) < 0)
	{
		PRINT_LOG("Could not open codec  !\n");
		return RETURN_VALUE_FAILED; // Could not open codec  
	} //<！打开解码器

	//<！Allocate Audio frame  
	m_pVideoState->m_pAudioFrame = av_frame_alloc();
	if (m_pVideoState->m_pAudioFrame == NULL)
	{
		PRINT_LOG("m_pAudioFrame is NULL  !\n");
		return RETURN_VALUE_FAILED;
	}

	////Out Audio Param
	//int32_t _defaultChannels = av_get_channel_layout_nb_channels(m_pVideoState->m_pAudioCodecCtx->channel_layout);
	//m_pVideoState->out_channels = _defaultChannels > 2 ? 2 : _defaultChannels;
	//m_pVideoState->out_channel_layout = av_get_default_channel_layout(m_pVideoState->out_channels); //<! 任何时候都需要通过声道数查找布局方式 !>
	////<! SDL最大支持2个声道，所以ffmpeg在转码的时候应该将多声道的转化为双声道 !>

	//m_pVideoState->out_sample_rate = m_pVideoState->m_pAudioCodecCtx->sample_rate;

	//m_pVideoState->out_nb_samples = m_pVideoState->m_pAudioCodecCtx->frame_size > 0 ? m_pVideoState->m_pAudioCodecCtx->frame_size:FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(m_pVideoState->out_sample_rate / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
	//
	////<! frame_size 可能为0 ， 后面的计算方式是从ffplay中拿来的，将SDL_AUDIO_MAX_CALLBACKS_PER_SEC修改为60后，测试视频正常，为什么这样计算未知。 !>
	//m_pVideoState->out_sample_fmt = AV_SAMPLE_FMT_S16;
	////Out Buffer Size
	//m_pVideoState->out_buffer_size = av_samples_get_buffer_size(
	//	&m_pVideoState->out_line_size,
	//	m_pVideoState->out_channels,
	//	m_pVideoState->out_nb_samples,
	//	m_pVideoState->out_sample_fmt,
	//	0
	//	);
	//m_pVideoState->out_bytes_per_sec = m_pVideoState->m_pAudioCodecCtx->sample_rate * m_pVideoState->out_channels * 2;  //<! 计算音频每秒字节数 !>
	//m_pVideoState->audio_delay = (double)(m_pVideoState->out_buffer_size) / (double)(m_pVideoState->out_bytes_per_sec);

	//m_pVideoState->au_convert_ctx = swr_alloc();
	//m_pVideoState->au_convert_ctx = swr_alloc_set_opts(
	//	m_pVideoState->au_convert_ctx,
	//	m_pVideoState->out_channel_layout,
	//	m_pVideoState->out_sample_fmt,
	//	m_pVideoState->out_sample_rate,
	//	/*m_pVideoState->m_pAudioCodecCtx->channel_layout*/ av_get_default_channel_layout(m_pVideoState->m_pAudioCodecCtx->channels),
	//	m_pVideoState->m_pAudioCodecCtx->sample_fmt,
	//	m_pVideoState->m_pAudioCodecCtx->sample_rate,
	//	0,
	//	NULL
	//	);
	//if (swr_init(m_pVideoState->au_convert_ctx) < 0)
	//{
	//	PRINT_LOG("swr_init() failed !\n");
	//	return RETURN_VALUE_FAILED;
	//}

	PRINT_LOG("Init Decoder audio successfully \n");
	return RETURN_VALUE_SUCCESS;
}
/*

int32_t Ffmpeg::InitThread()
{

m_pVideoState->m_pReadThread = SDL_CreateThread(&Ffmpeg::ReadThread, "ReadThread", this);  //<! 读取线程
CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pReadThread, "ReadThread null ptr");
m_pVideoState->m_pAudioThread = SDL_CreateThread(&Ffmpeg::AudioThread, "AudioThread", this); //!< 音频播放线程
CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pAudioThread, "AudioThread null ptr");
m_pVideoState->m_pVideoThread = SDL_CreateThread(&Ffmpeg::VideoThread, "VideoThread", this); //<! 视频播放线程
CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_pVideoThread, "VideoThread null ptr");

return RETURN_VALUE_SUCCESS;
}
*/

#if USE_SDL_VIDEO
SDL_Rect Ffmpeg::RecalculateWidthHeightRatio(FfVideoState* vs)
{
	SDL_Rect  rect;
	uint32_t w = 0;
	uint32_t h = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t frameWidth = vs->m_iFilmWidth;
	uint32_t frameHeight = vs->m_iFilmHeight;
	uint32_t windowWidth = m_iWndWidth;
	uint32_t windowHeight = m_iWndHeight;


	if ((frameWidth >= windowWidth) || (frameHeight >= windowHeight))
	{
		w = windowWidth;
		h = windowHeight;
	}//<!若视频宽高都大于或等于显示界面，依照显示界面显示
	else if ((frameWidth < windowWidth) || (frameHeight < windowHeight))
	{
		w = frameWidth;
		h = frameHeight;
		x = ceil((windowWidth - frameWidth) / 2);
		y = ceil((windowHeight - frameHeight) / 2);
	}//<!若视频宽高都小于显示界面，依照视频宽高显示，并计算位置
	   //////<!计算视频宽高比的问题暂时不解决，统一使用窗口尺寸   $Nico@2016-11-3 16:56:35


	rect.w = w;
	rect.h = h;
	rect.x = x;
	rect.y = y;

	return rect;
}
#endif

int32_t Ffmpeg::InitAudioQueue(AudioQueue **aq) {
	if (*aq == nullptr)
	{
		*aq = (AudioQueue *)av_malloc(sizeof(AudioQueue));
	}
	memset(*aq, 0, sizeof(AudioQueue));

	(*aq)->audioMutex = SDL_CreateMutex();
	CHECK_PTR_RETURN_IF_NULL((*aq)->audioMutex, "audioMutex is null! \n", RETURN_VALUE_FAILED);
	(*aq)->audioCond = SDL_CreateCond();
	CHECK_PTR_RETURN_IF_NULL((*aq)->audioCond, "audioCond is null! \n", RETURN_VALUE_FAILED);

	return RETURN_VALUE_SUCCESS;
}

void Ffmpeg::ReleaseAudioQueue(AudioQueue **aq) {

	if (*aq != nullptr)
	{

		SDL_DestroyMutex((*aq)->audioMutex);
		SDL_DestroyCond((*aq)->audioCond);

		av_free(*aq);
	}
}
uint32_t Ffmpeg::PutItemIntoAudioQueue(AudioQueue *aq, AudioQueueItem *item)
{
	uint32_t result = 0;

	SDL_LockMutex(aq->audioMutex);//加锁

	if (aq->Length < AudioBufferMaxSize)
	{
		if (!aq->FirstItem)//第一个item为null 则队列是空的
		{
			aq->FirstItem = item;
			aq->LastItem = item;
			aq->Length = 1;
		}
		else
		{
			aq->LastItem->Next = item;//添加到队列后面
			aq->Length++;
			aq->LastItem = item;//此item变成队列尾部
		}
		//this->m_pVideoState->m_dCurrentCacheAudioPts = item->Pts;
		if (aq->Length >= AudioBufferMinSize)
		{
			SDL_CondSignal(aq->audioCond);//唤醒其他线程  如果缓冲区里面有几个数据后再唤醒 较好
		}
		result = 1;
	}
	else///音频缓冲区的大小 大于设定值 则让线程等待
	{
		//PRINT_LOG("Audio Put Thread wait\n");
		SDL_CondWait(aq->audioCond, aq->audioMutex);//解锁  等待被唤醒
		//SDL_CondWaitTimeout(aq->audioCond, aq->audioMutex, SDL_THREAD_WAITTIMEOUT);

	}

	SDL_UnlockMutex(aq->audioMutex);//解锁

	return result;
}

uint32_t Ffmpeg::GetItemFromAudioQueue(AudioQueue* aq, AudioQueueItem** item)
{
	uint32_t result = 0;

	SDL_LockMutex(aq->audioMutex);

	if (aq->Length > 0)
	{
		if (aq->FirstItem)//有数据
		{
			*item = aq->FirstItem;
			if (!aq->FirstItem->Next)//只有一个
			{
				aq->FirstItem = NULL;
				aq->LastItem = NULL;
			}
			else
			{
				aq->FirstItem = aq->FirstItem->Next;
			}
			aq->Length--;
			(*item)->Next = NULL;
			result = 1;
		}
		//if (aq->Length <= AudioBufferMinSize)
		//{
		SDL_CondSignal(aq->audioCond);//唤醒下载线程
		//}
	}
	else
	{
		//PRINT_LOG("Audio Get Thread wait\n");
		SDL_CondWait(aq->audioCond, aq->audioMutex);//解锁  等待被唤醒
		//SDL_CondWaitTimeout(aq->audioCond, aq->audioMutex, SDL_THREAD_WAITTIMEOUT);


	}

	SDL_UnlockMutex(aq->audioMutex);

	return result;
}


int32_t Ffmpeg::SetMovieReplay(ReplayType replayType)
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	m_pVideoState->m_iMovieReplayFlag = replayType;    //<! 当前无效 !>
	return RETURN_VALUE_SUCCESS;
}

void Ffmpeg::ClearAudioQueue(AudioQueue *aq)
{
	if (nullptr == aq)
	{
		return;
	}

	AudioQueueItem *item, *temp;

	SDL_LockMutex(aq->audioMutex);

	for (item = aq->FirstItem; item != NULL; item = temp)
	{
		temp = item->Next;//
		av_free(item->AudioData);//释放video里面的数据
		av_free(item);
		aq->Length--;
	}
	aq->FirstItem = NULL;
	aq->LastItem = NULL;

	SDL_UnlockMutex(aq->audioMutex);

}

int32_t Ffmpeg::InitVideoQueue(VideoQueue **vq)
{
	if (*vq == nullptr)
	{
		*vq = (VideoQueue *)av_malloc(sizeof(VideoQueue));
	}
	memset(*vq, 0, sizeof(VideoQueue));//初始化首地址为0

	(*vq)->videoMutex = SDL_CreateMutex();
	CHECK_PTR_RETURN_IF_NULL((*vq)->videoMutex, "videoMutex is null! \n", RETURN_VALUE_FAILED);
	(*vq)->videoCond = SDL_CreateCond();
	CHECK_PTR_RETURN_IF_NULL((*vq)->videoCond, "videoCond is null! \n", RETURN_VALUE_FAILED);


	return RETURN_VALUE_SUCCESS;
}

void Ffmpeg::ReleaseVideoQueue(VideoQueue **vq) {

	if (*vq != nullptr)
	{

		SDL_DestroyMutex((*vq)->videoMutex);
		SDL_DestroyCond((*vq)->videoCond);

		av_free(*vq);
	}
}

uint32_t Ffmpeg::PutItemIntoVideoQueue(VideoQueue *vq, VideoQueueItem *item)
{
	uint32_t result = 0;

	SDL_LockMutex(vq->videoMutex);//加锁

	if (vq->Length < VideoBufferMaxSize)
	{
		if (!vq->FirstItem)//第一个item为null 则队列是空的
		{
			vq->FirstItem = item;
			vq->LastItem = item;
			vq->Length = 1;
			vq->BufferPts = item->Pts;
		}
		else
		{
			vq->LastItem->Next = item;//添加到队列后面
			vq->Length++;
			vq->LastItem = item;//此item变成队列尾部
			vq->BufferPts = item->Pts;
		}

		//this->m_pVideoState->m_dCurrentCacheVideoPts = item->Pts;
		if (vq->Length >= VideoBufferMinSize)
		{
			SDL_CondSignal(vq->videoCond);//唤醒其他线程  如果缓冲区里面有几个数据后再唤醒 较好
		}
		result = 1;
	}
	else
	{
		//PRINT_LOG("Video Put Thread wait\n");
		SDL_CondWait(vq->videoCond, vq->videoMutex);//解锁  等待被唤醒
		//SDL_CondWaitTimeout(vq->videoCond, vq->videoMutex, SDL_THREAD_WAITTIMEOUT);

	}

	//PRINT_LOG("video queue : %d", vq->Length);

	SDL_UnlockMutex(vq->videoMutex);//解锁
	return result;
}

uint32_t Ffmpeg::GetItemFromVideoQueue(VideoQueue* vq, VideoQueueItem** item)
{
	uint32_t result = 0;

	SDL_LockMutex(vq->videoMutex);

	if (vq->Length > 0)
	{
		if (vq->FirstItem)//有数据
		{
			*item = vq->FirstItem;
			if (!vq->FirstItem->Next)//只有一个
			{
				vq->FirstItem = NULL;
				vq->LastItem = NULL;
			}
			else
			{
				vq->FirstItem = vq->FirstItem->Next;
			}
			vq->Length--;
			(*item)->Next = NULL;
			result = 1;
		}
		//if (vq->Length <= VideoBufferMinSize)
		//{
		SDL_CondSignal(vq->videoCond);//唤醒下载线程
		//}
	}
	else
	{
		SDL_CondWait(vq->videoCond, vq->videoMutex);//解锁  等待被唤醒
		//SDL_CondWaitTimeout(vq->videoCond, vq->videoMutex, SDL_THREAD_WAITTIMEOUT);

	}


	SDL_UnlockMutex(vq->videoMutex);

	return result;
}

void Ffmpeg::ClearVideoQueue(VideoQueue *vq)
{
	if (nullptr == vq)
	{
		return;
	}


	VideoQueueItem *item, *temp;

	SDL_LockMutex(vq->videoMutex);

	for (item = vq->FirstItem; item != NULL; item = temp)
	{
		temp = item->Next;//
		//av_free(item->VideoData);//释放video里面的数据
		//SetGPUMemEnable(item->D3dVideoData);
		item->Texture = nullptr;
		av_free(item);
		vq->Length--;
	}
	vq->FirstItem = NULL;
	vq->LastItem = NULL;
	SDL_UnlockMutex(vq->videoMutex);
}

double Ffmpeg::GetPts(AVPacket* packet, AVFrame* frame, AVRational timeBase)
{
	double pts = 0;
	if (packet->dts != AV_NOPTS_VALUE) {
		pts = av_frame_get_best_effort_timestamp(frame);
	}
	else {
		pts = 0;
	}
	pts *= av_q2d(timeBase);
	return pts;
}

int32_t Ffmpeg::FirstFrameCheck(FirstFrame* first_frame, double pts)
{
	if (IsFirstFrame::FIRST_FRAME == first_frame->isFirstFrame)
	{
		if (pts >= 0)
		{
			first_frame->FirstFramePts = pts;
		}

		first_frame->isFirstFrame = IsFirstFrame::NOT_FIRST_FRAME;
	}

	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::GetDelay(FfVideoState* Ffvs)
{

	double _video_audio_current_diff = Ffvs->m_dCurrentVideoPts - Ffvs->m_dCurrentAudioPts;

	//PRINT_LOG("_video_audio_current_diff:%f", _video_audio_current_diff);
	int32_t _total_delay = 0, _diff_delay = 0;

	if ((fabs(_video_audio_current_diff) < MIDDLE_VIDEO_AUDIO_DIFFER_TIME) && (fabs(_video_audio_current_diff) > MIN_VIDEO_AUDIO_DIFFER_TIME))   //<! 差异大于这个数时，进行同步，否则继续播放 !>  
	{
		_diff_delay = ceil(_video_audio_current_diff * 20);   //<! _video_audio_current_diff * 1000 / 20 !>			
	}
	else if ((fabs(_video_audio_current_diff) < MAX_VIDEO_AUDIO_DIFFER_TIME) && (fabs(_video_audio_current_diff) >= MIDDLE_VIDEO_AUDIO_DIFFER_TIME))
	{
		_diff_delay = ceil(_video_audio_current_diff * 50);   //<! _video_audio_current_diff * 1000 / 10 !>			
	}
	else if (fabs(_video_audio_current_diff) > MAX_VIDEO_AUDIO_DIFFER_TIME)
	{
		_diff_delay = ceil(_video_audio_current_diff * 100);
	}

	_diff_delay = _diff_delay > (Ffvs->m_iFrameDelay >> 1) ? (Ffvs->m_iFrameDelay >> 1) : _diff_delay;
	//_diff_delay = _diff_delay > 10 ? 10 : _diff_delay;

	int32_t _memcpy_time_cost = 0;

#if USE_SDL_VIDEO
	_memcpy_time_cost = (_video_audio_current_diff < 0 ? Ffvs->m_iMemcpyCostTime : 0)
#endif

		_total_delay = Ffvs->m_iFrameDelay + _diff_delay + _memcpy_time_cost;
	_total_delay = _total_delay < 0 ? 0 : _total_delay;

	return _total_delay;
}

int32_t Ffmpeg::CreateVideoItemFromPacket(AVPacket* packet)
{

	//int64_t begin_time = av_gettime_relative();
	//OutputDebugString(L"Start decoding frame\n");
	FfVideoState* Ffvs = m_pVideoState;

	int32_t _got_frame = 0;

	int32_t ret = avcodec_decode_video2(
		Ffvs->m_pVideoCodecCtx,
		Ffvs->m_pVideoFrame,
		&_got_frame,
		packet
	);
	//PRINT_LOG("Decode time : %ld", av_gettime_relative() - begin_time);
	if (ret < 0)
	{
		PRINT_LOG("Error in decoding audio frame.\n");
		return RETURN_VALUE_FAILED;
	}


	if (_got_frame)
	{
		double pts = GetPts(packet, Ffvs->m_pVideoFrame, Ffvs->m_pFormatCtx->streams[Ffvs->m_iVideoStream]->time_base);

		//FirstFrameCheck(&(Ffvs->m_sFirstVideoFrame), pts);

		PictureTexture* _Texture = nullptr;

		do
		{
			_Texture = FindEnabledGPUMem();
		} while (nullptr == _Texture && (Ffvs->m_iPlayingFlag != PlayStatus::STOP));

		if (nullptr == _Texture)
		{
			return RETURN_VALUE_FAILED;
		}

#if USE_SDL_VIDEO
		uint8_t* raw_data = (uint8_t*)av_mallocz(Ffvs->m_iBufferSizeInBytes);
		CHECK_PTR_RETURN_IF_NULL(raw_data, "raw_data is null! \n", RETURN_VALUE_FAILED);
		_Texture->raw_pixs = raw_data;

		av_image_fill_arrays(
			Ffvs->m_pFrameYUV->data,
			Ffvs->m_pFrameYUV->linesize,
			_Texture->raw_pixs,
			AV_FORMAT_CURRENT_USED,
			Ffvs->m_iRenderTargetWidth,
			Ffvs->m_iRenderTargetHeight,
			1
		);
#endif
		//int64_t begin_time = av_gettime_relative();
		Ffvs->m_pSws_ctx = sws_getCachedContext(
			Ffvs->m_pSws_ctx,
			Ffvs->m_pVideoCodecCtx->width,
			Ffvs->m_pVideoCodecCtx->height,
			Ffvs->m_pVideoCodecCtx->pix_fmt,
			Ffvs->m_iFilmWidth,
			Ffvs->m_iFilmHeight,
			AV_FORMAT_CURRENT_USED,
			SWS_FAST_BILINEAR,
			NULL,
			NULL,
			NULL
		);
		CHECK_PTR_RETURN_IF_NULL(Ffvs->m_pSws_ctx, "m_pSws_ctx is null! \n", RETURN_VALUE_FAILED);

		sws_scale(
			Ffvs->m_pSws_ctx,
			(const uint8_t* const*)Ffvs->m_pVideoFrame->data,
			Ffvs->m_pVideoFrame->linesize,
			0,
			Ffvs->m_pVideoCodecCtx->height,
			Ffvs->m_pFrameYUV->data,
			Ffvs->m_pFrameYUV->linesize
		);

		//PRINT_LOG("scale time : %ld", av_gettime_relative() - begin_time);


		D3D11_BOX destRegion;
		destRegion.left = 0;
		destRegion.top = 0;
		destRegion.right = Ffvs->m_iFilmWidth / 2;
		destRegion.bottom = Ffvs->m_iFilmHeight;
		destRegion.front = 0;
		destRegion.back = 1;


		//int64_t begin_time = av_gettime_relative();
		Ffvs->m_pD11DeviceContext->UpdateSubresource((ID3D11Texture2D*)_Texture->rt->LRT, 0, &destRegion, Ffvs->m_pFrameYUV->data[0], Ffvs->m_iFilmWidth << 2, 0);
		Ffvs->m_pD11DeviceContext->UpdateSubresource((ID3D11Texture2D*)_Texture->rt->RRT, 0, &destRegion, Ffvs->m_pFrameYUV->data[0] + (Ffvs->m_iFilmWidth << 1), Ffvs->m_iFilmWidth << 2, 0);
		//PRINT_LOG("d3d11 update time : %ld", av_gettime_relative() - begin_time);

		//PRINT_LOG("Update start");

		//PRINT_LOG("Update end");
		//static int ii = 0;
		//{
		//	ID3D11Resource* SaveTextureResource;
		//	HRESULT hr;

		//	wchar_t path[MAX_PATH];

		//	swprintf(path, L"D:\\Test%d.jpg",ii);

		//	//std::wstring path1 = L"D:\\Test" + _itow(ii);

		//	hr = ((ID3D11Texture2D*)(_Texture->rt->LRT))->QueryInterface(__uuidof(ID3D11Resource), (void**)&SaveTextureResource);
		//	hr = D3DX11SaveTextureToFile(Ffvs->m_pD11DeviceContext, SaveTextureResource, D3DX11_IFF_JPG,path);
		//	DWORD err = GetLastError();

		//}

		//创建视频item
		VideoQueueItem *videoItem = nullptr;
		videoItem = (VideoQueueItem *)av_mallocz(sizeof(VideoQueueItem));

		CHECK_PTR_RETURN_IF_NULL(videoItem, "Video item malloc falied!\n", RETURN_VALUE_FAILED);

		videoItem->Height = Ffvs->m_iFilmHeight;
		videoItem->Width = Ffvs->m_iFilmWidth;
		videoItem->Texture = _Texture;
		videoItem->Length = Ffvs->m_pFrameYUV->linesize[0];
		//获取显示时间戳pts
		videoItem->Pts = SynchronizeVideo(Ffvs, Ffvs->m_pVideoFrame, pts);
		videoItem->Next = NULL;
		while (!PutItemIntoVideoQueue(Ffvs->m_VideoQueue, videoItem) && (Ffvs->m_iPlayingFlag != PlayStatus::STOP));
	}
	//PRINT_LOG("decode time : %ld", av_gettime_relative() - begin_time);
	av_frame_unref(Ffvs->m_pVideoFrame);

	return RETURN_VALUE_SUCCESS;
}

double Ffmpeg::SynchronizeVideo(FfVideoState *is, AVFrame *src_frame, double pts) {

	double frame_delay = 0, extra_delay = 0;

	if (pts >= 0) {
		/* if we have pts, set video clock to it */
		is->m_dCurrentDecodeVideoPts = pts;
		//return (double)0;
	}
	else {
		/* if we aren't given a pts, set it to the clock */
		pts = is->m_dCurrentDecodeVideoPts;
	}

	if (is->m_iFPS > 0)
	{
		/* update the video clock */
		frame_delay = (double)1 / is->m_iFPS;
		/* if we are repeating a frame, adjust clock accordingly */
		extra_delay = src_frame->repeat_pict / (is->m_iFPS * 2);
	}
	else
	{
		/* update the video clock */
		frame_delay = av_q2d(is->m_pVideoCodecCtx->time_base);
		/* if we are repeating a frame, adjust clock accordingly */
		extra_delay = src_frame->repeat_pict * (frame_delay);
	}

	is->m_dCurrentDecodeVideoPts += frame_delay + extra_delay; //<! 预测下一个视频的时间 !>

	return pts;
}

double Ffmpeg::SynchronizeAudio(FfVideoState *is, AVFrame *src_frame, double pts) {


	if (pts >= 0) {
		/* if we have pts, set audio clock to it */
		is->m_dCurrentDecodeAudioPts = pts;
		//return (double)0;
	}
	else {
		/* if we aren't given a pts, set it to the clock */
		pts = is->m_dCurrentDecodeAudioPts;
	}

	//if (pts == (double)0)
	//{
	//	return (double)0;
	//}

	is->m_dCurrentDecodeAudioPts += (is->audio_delay);  //<! 预测下一个音频的时间 !>   //<! 增加视频解码的时间作整体偏移 !>

	return 	is->m_dCurrentDecodeAudioPts;  //<! 预测下一个音频的时间 !>   //<! 增加视频解码的时间作整体偏移 !>
}


int32_t Ffmpeg::CreateAudioItemFromPacket(AVPacket* packet)
{

	FfVideoState* Ffvs = m_pVideoState;

	int32_t _got_frame = 0;

	uint32_t ret = avcodec_decode_audio4(
		Ffvs->m_pAudioCodecCtx,
		Ffvs->m_pAudioFrame,
		&_got_frame,
		packet
	);


	if (ret < 0)
	{
		printf("Error in decoding audio frame.\n");
		return RETURN_VALUE_FAILED;
	}

	if (_got_frame)
	{

		double pts = GetPts(packet, Ffvs->m_pAudioFrame, Ffvs->m_pFormatCtx->streams[Ffvs->m_iAudioStream]->time_base);


		//int32_t resampled_data_size;
		int64_t dec_channel_layout;
		//int32_t wanted_nb_samples;



		dec_channel_layout =
			(Ffvs->m_pAudioFrame->channel_layout && av_frame_get_channels(Ffvs->m_pAudioFrame) == av_get_channel_layout_nb_channels(Ffvs->m_pAudioFrame->channel_layout)) ?
			Ffvs->m_pAudioFrame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(Ffvs->m_pAudioFrame));
		//wanted_nb_samples = Ffvs->m_pAudioFrame->nb_samples; //Ffvs->m_SrcAudioParams.nb_samples;


		if (AVSampleFormat(Ffvs->m_pAudioFrame->format) != Ffvs->m_SrcAudioParams.fmt ||
			dec_channel_layout != Ffvs->m_SrcAudioParams.channel_layout ||
			Ffvs->m_pAudioFrame->sample_rate != Ffvs->m_SrcAudioParams.freq ||
			(Ffvs->m_pAudioFrame->nb_samples != Ffvs->m_SrcAudioParams.nb_samples && !Ffvs->au_convert_ctx)) {



			PRINT_LOG("Reinit SDL audio!");
			InitSDLAudio(dec_channel_layout, av_frame_get_channels(Ffvs->m_pAudioFrame), Ffvs->m_pAudioFrame->sample_rate, Ffvs->m_pAudioFrame->nb_samples);


			PRINT_LOG("Reinit swr!");
			swr_free(&Ffvs->au_convert_ctx);
			Ffvs->au_convert_ctx = swr_alloc_set_opts(
				NULL,
				Ffvs->m_OutAudioParams.channel_layout, Ffvs->m_OutAudioParams.fmt, Ffvs->m_OutAudioParams.freq,
				dec_channel_layout, AVSampleFormat(Ffvs->m_pAudioFrame->format), Ffvs->m_pAudioFrame->sample_rate,
				0, NULL);


			if (!Ffvs->au_convert_ctx || swr_init(Ffvs->au_convert_ctx) < 0) {
				PRINT_LOG(
					"Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
					Ffvs->m_pAudioFrame->sample_rate, av_get_sample_fmt_name(AVSampleFormat(Ffvs->m_pAudioFrame->format)), av_frame_get_channels(Ffvs->m_pAudioFrame),
					Ffvs->m_OutAudioParams.freq, av_get_sample_fmt_name(Ffvs->m_OutAudioParams.fmt), Ffvs->m_OutAudioParams.channels);
				swr_free(&Ffvs->au_convert_ctx);
				return RETURN_VALUE_FAILED;
			}

			Ffvs->m_SrcAudioParams.channel_layout = dec_channel_layout;
			Ffvs->m_SrcAudioParams.channels = av_frame_get_channels(Ffvs->m_pAudioFrame);
			Ffvs->m_SrcAudioParams.freq = Ffvs->m_pAudioFrame->sample_rate;
			Ffvs->m_SrcAudioParams.fmt = AVSampleFormat(Ffvs->m_pAudioFrame->format);
			Ffvs->m_SrcAudioParams.nb_samples = Ffvs->m_pAudioFrame->nb_samples; //<!--  -->
		}

		//int32_t out_count = (int64_t)wanted_nb_samples * Ffvs->m_OutAudioParams.freq / Ffvs->m_pAudioFrame->sample_rate + 256;
		//int32_t out_size = av_samples_get_buffer_size(NULL, Ffvs->m_OutAudioParams.channels, wanted_nb_samples, Ffvs->m_OutAudioParams.fmt, 0);
		int32_t out_size = av_samples_get_buffer_size(NULL, Ffvs->m_OutAudioParams.channels, Ffvs->m_OutAudioParams.nb_samples, Ffvs->m_OutAudioParams.fmt, 0);


		uint8_t* out = nullptr;


		if (Ffvs->au_convert_ctx)
		{
			const uint8_t **in = (const uint8_t **)Ffvs->m_pAudioFrame->extended_data;
			int32_t len2;
			if (out_size < 0) {
				PRINT_LOG("av_samples_get_buffer_size() failed\n");
				return -1;
			}
			if (Ffvs->m_OutAudioParams.nb_samples != Ffvs->m_pAudioFrame->nb_samples) {
				if (swr_set_compensation(Ffvs->au_convert_ctx, (Ffvs->m_OutAudioParams.nb_samples - Ffvs->m_pAudioFrame->nb_samples) * Ffvs->m_OutAudioParams.freq / Ffvs->m_pAudioFrame->sample_rate,
					Ffvs->m_OutAudioParams.nb_samples * Ffvs->m_OutAudioParams.freq / Ffvs->m_pAudioFrame->sample_rate) < 0) {
					PRINT_LOG("swr_set_compensation() failed\n");
					return -1;
				}
			}
			out = (uint8_t*)av_mallocz(out_size);
			CHECK_PTR_RETURN_IF_NULL(out, "av_mallocz() in audio falied!\n", RETURN_VALUE_FAILED);

			//out = (uint8_t*)av_malloc(Ffvs->m_OutAudioParams.frame_size);

			//unsigned int out_size_real;
			//av_fast_malloc(&out, &out_size_real, out_size);
			//if (!out)
			//	return AVERROR(ENOMEM);
			len2 = swr_convert(Ffvs->au_convert_ctx, &out, Ffvs->m_OutAudioParams.nb_samples, in, Ffvs->m_pAudioFrame->nb_samples);
			if (len2 < 0) {
				PRINT_LOG("swr_convert() failed\n");
				return -1;
			}
		}
		else
		{
			int32_t data_size = av_samples_get_buffer_size(
				NULL,
				av_frame_get_channels(Ffvs->m_pAudioFrame),
				Ffvs->m_pAudioFrame->nb_samples,
				AVSampleFormat(Ffvs->m_pAudioFrame->format),
				1);
			out = (uint8_t*)av_memdup(Ffvs->m_pAudioFrame->data[0], data_size);
			CHECK_PTR_RETURN_IF_NULL(out, "av_memdup() in audio falied!\n", RETURN_VALUE_FAILED);
		}


		m_pVideoState->audio_delay = (double)(Ffvs->m_OutAudioParams.nb_samples) / (double)(Ffvs->m_OutAudioParams.freq);
		//resampled_data_size = len2 * Ffvs->m_OutAudioParams.channels * av_get_bytes_per_sample(Ffvs->m_OutAudioParams.fmt);

		AudioQueueItem *audioItem = (AudioQueueItem *)av_malloc(sizeof(AudioQueueItem));

		CHECK_PTR_RETURN_IF_NULL(audioItem, "Audio item malloc falied!\n", RETURN_VALUE_FAILED);

		audioItem->AudioData = out;
		audioItem->Length = out_size;
		audioItem->Pts = SynchronizeAudio(Ffvs, Ffvs->m_pAudioFrame, pts);
		audioItem->Next = NULL;
		while (!PutItemIntoAudioQueue(Ffvs->m_AudioQueue, audioItem) && (Ffvs->m_iPlayingFlag != PlayStatus::STOP));

	}
	av_frame_unref(Ffvs->m_pAudioFrame);

	return RETURN_VALUE_SUCCESS;
}

int Ffmpeg::SetCyclePlay(bool iscycle)
{
	//if (iscycle)
	//{
	//	m_pVideoState->m_iCycleFlag = CYCLEPLAY;
	//}
	//else
	//{
	//	m_pVideoState->m_iCycleFlag = LISTPLAY;
	//}

	return RETURN_VALUE_SUCCESS;
}

/*
int Ffmpeg::AudioThread(void* ptr)
{
Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
FfVideoState* Ffvs = instance->m_pVideoState;

//AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));


while (Ffvs->m_iPlayingFlag != STOP)
{

//PRINT_LOG("AudioThread\n");
if (Ffvs->m_iPlayingFlag == PAUSE)
{

//SDL_Delay(1);
continue;
}
else if (Ffvs->m_iPlayingFlag == PLAY)
{

}

if (Ffvs->m_dCurrentAudioPts >= Ffvs->m_dCurrentVideoPts + 0.18)
{
if (Ffvs->m_VideoQueue->Length > 0 && Ffvs->m_AudioQueue->Length < AudioBufferMaxSize)
{
SDL_Delay(1);
continue;
}
}


AudioItem* audioItem = nullptr;
//从队列中拿出音频数据
if (instance->GetItemFromAudioQueue(Ffvs->m_AudioQueue, &audioItem))
{
Ffvs->m_dCurrentAudioPts = audioItem->Pts;//当前视频时间戳

Ffvs->audio_len = audioItem->Length;
Ffvs->audio_pos = audioItem->AudioData;

SDL_PauseAudio(0);
while (Ffvs->audio_len > 0 && Ffvs->m_iPlayingFlag != STOP)   //Wait until play finished  //如果停止，则不需要等待,因为当STOP时，AudioCallback已经停止。
SDL_Delay(1);

av_free(audioItem->AudioData);
av_free(audioItem);

}
}



SDL_CondSignal(Ffvs->m_AudioQueue->audioCond);
PRINT_LOG("AudioThread Returned.\n");

return RETURN_VALUE_SUCCESS;
}*/

/*
int Ffmpeg::VideoThread(void* ptr)
{

Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
FfVideoState* Ffvs = instance->m_pVideoState;

while (Ffvs->m_iPlayingFlag != STOP)
{

//PRINT_LOG("VideoThread\n");
if (Ffvs->m_iPlayingFlag == PAUSE)
{

//SDL_Delay(1);
continue;
}
else if (Ffvs->m_iPlayingFlag == PLAY)
{
}

if (Ffvs->m_dCurrentVideoPts >= Ffvs->m_dCurrentAudioPts + 0.18)
{
if (Ffvs->m_AudioQueue->Length > 0 && Ffvs->m_VideoQueue->Length < VideoBufferMaxSize)
{
SDL_Delay(1);
continue;
}
}


VideoItem* videoItem = nullptr;
//从队列中拿出视频数据
//PRINT_LOG("Get Video Item Start\n");
if (instance->GetItemFromVideoQueue(Ffvs->m_VideoQueue, &videoItem))
{

//PRINT_LOG("Get Video Item End\n");
Ffvs->m_dCurrentVideoPts = videoItem->Pts;//当前视频时间戳

//SDL---------------------------
#if USE_SDL_VIDEO
int ret = SDL_UpdateTexture(
Ffvs->m_pBmp,
&Ffvs->m_rect,
videoItem->VideoData,
videoItem->Length
);

ret = SDL_RenderClear(Ffvs->m_pRender);
SDL_Rect rect = instance->RecalculateWidthHeightRatio(Ffvs);//获取显示尺寸
ret = SDL_RenderCopy(Ffvs->m_pRender, Ffvs->m_pBmp, &Ffvs->m_rect, &rect);
SDL_RenderPresent(Ffvs->m_pRender);
#endif
//SDL End-----------------------

SDL_LockMutex(Ffvs->m_pGetFrameMutex);
//SDL_memcpy(Ffvs->m_pVideoBufferForUnity, videoItem->VideoData, (Ffvs->m_iBufferSizeInBytes));
//Ffvs->m_pVideoBufferForUnity = videoItem->VideoData;
//PRINT_LOG("Copy Resource Start\n");
//Ffvs->m_pD11DeviceContext->Flush();
Ffvs->m_pD11DeviceContext->CopyResource(Ffvs->m_pGPUTempBuffer, videoItem->VideoData);
Ffvs->m_pD11DeviceContext->Flush();

//PRINT_LOG("Copy Resource End\n");

SDL_UnlockMutex(Ffvs->m_pGetFrameMutex);

instance->SetGPUMemEnable(videoItem->VideoData);
av_free(videoItem);
Ffvs->m_iLastFrameUploaded++;

}

}

//Ffvs->m_VideoQueue = NULL;

//<!

SDL_CondSignal(Ffvs->m_VideoQueue->videoCond);

PRINT_LOG("VideoThread Returned.\n");

return RETURN_VALUE_SUCCESS;
}
*/


double Ffmpeg::CalculateDifferTime(double currentVideoPts, double currentAudioPts)
{

	return 0;
}
/*

int Ffmpeg::ReadThread(void* ptr)
{
//!<start,  play the video.   $Nico@2016-11-2 09:42:59
//!<
//!<
Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
FfVideoState* Ffvs = instance->m_pVideoState;
int32_t isEnd = 0;

while (Ffvs->m_iPlayingFlag != STOP)
{
//PRINT_LOG("ReadThread\n");

if (Ffvs->m_iSeekFlag == SEEK_TIME)
{
if (Ffvs->m_iBufferCleanFlag == BUFFER_CLEAN)
{
instance->ClearVideoQueue(Ffvs->m_VideoQueue);
instance->ClearAudioQueue(Ffvs->m_AudioQueue);
avformat_flush(Ffvs->m_pFormatCtx);
}
double ret = av_seek_frame(
Ffvs->m_pFormatCtx,
RETURN_VALUE_FAILED,
(Ffvs->m_iSeekTime) * AV_TIME_BASE,
AVSEEK_FLAG_BACKWARD
);
Ffvs->m_iSeekFlag = SEEK_NOTHING;
continue;
}

if (Ffvs->m_iPlayingFlag == PAUSE)
{

SDL_Delay(1);
continue;
}

isEnd = av_read_frame(Ffvs->m_pFormatCtx, Ffvs->m_pflush_pkt);


if (isEnd < 0)
{
instance->SeekTo(0, BUFFER_NO_CLEAN);
}

//double _time;
if (Ffvs->m_pflush_pkt->stream_index == Ffvs->m_iVideoStream)
{
instance->CreateVideoItemFromPacket();
}
else if (Ffvs->m_pflush_pkt->stream_index == Ffvs->m_iAudioStream)
{
instance->CreateAudioItemFromPacket();
}

av_packet_unref(Ffvs->m_pflush_pkt);

}
//!<end,  play the video.   $Nico@2016-11-2 09:42:59
//!<
//!<

SDL_CondSignal(Ffvs->m_AudioQueue->audioCond);
SDL_CondSignal(Ffvs->m_VideoQueue->videoCond);

SDL_WaitThread(Ffvs->m_pAudioThread, NULL);
SDL_WaitThread(Ffvs->m_pVideoThread, NULL);


//!<
PRINT_LOG("ReadThread Returned.\n");

return RETURN_VALUE_SUCCESS;
}
*/





int Ffmpeg::LockMgr(void **mtx, enum AVLockOp op)
{
	switch (op) {
	case AV_LOCK_CREATE:

		*mtx = SDL_CreateMutex();

		if (!*mtx)
			return 1;
		return 0;
	case AV_LOCK_OBTAIN:

		return !!SDL_LockMutex((SDL_mutex *)*mtx);

	case AV_LOCK_RELEASE:

		return !!SDL_UnlockMutex((SDL_mutex *)*mtx);

	case AV_LOCK_DESTROY:
		SDL_DestroyMutex((SDL_mutex *)*mtx);

		return 0;
	}
	return 1;
}


int Ffmpeg::InterruptCallBack(void* usrdata)
{
	FfVideoState* Ffvs = reinterpret_cast<FfVideoState*>(usrdata);

	if ((Ffvs->m_iPlayingFlag == PlayStatus::STOP)) //<! 停止播放 !>
	{
		return 1;
	}

	return 0;
}

/*          //<! 外部时钟的回调函数，效果不好，停用 !>
Uint32 Ffmpeg::SDL_Timer_Cb(Uint32 interval, void* param)
{
FfVideoState* Ffvs = reinterpret_cast<FfVideoState*>(param);

Ffvs->m_sExtClock.play_time += (double)interval / 1000;

return interval;
}*/

//void  Ffmpeg::SDL_AudioCallBack(void *userdata, uint8_t * stream, int len) {
//
//	SDL_memset(stream, 0, len); 
//	Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(userdata);
//	FfVideoState* vs = instance->m_pVideoState;
//	if (vs->audio_len == 0)		/*  Only  play  if  we  have  data  left  */
//	{
//		return;
//	}
//	len = (len > vs->audio_len ? vs->audio_len : len);	/*  Mix  as  much  data  as  possible  */
//
//	//int32_t count = len / 1024;
//	//int32_t last = len % 1024;
//	//double time_in_one_count = (double)1024 / vs->m_OutAudioParams.bytes_per_sec;
//	//while (count--)
//	//{
//	//	SDL_memset(stream, 0, len);
//	//	SDL_MixAudio(stream, vs->audio_pos, 1024, vs->m_iCurrentVolume);
//	//	vs->audio_pos += 1024;
//	//	vs->m_dCurrentAudioPts += time_in_one_count;
//	//}
//
//	//if (last != 0)
//	//{
//	//	SDL_memset(stream, 0, last);
//	//	SDL_MixAudio(stream, vs->audio_pos, last, vs->m_iCurrentVolume);
//	//	vs->audio_pos += last;
//	//	vs->m_dCurrentAudioPts += (double)last / vs->m_OutAudioParams.bytes_per_sec;
//	//}
//
//
//
//
//	SDL_MixAudio(stream, vs->audio_pos, len, vs->m_iCurrentVolume);
//
//	vs->audio_pos += len;
//	vs->audio_len -= len;
//
//	return;
//}


//<! 外部时钟的线程，效果不好，停用 !>
/*
int Ffmpeg::ExtClockThread(void* ptr)
{
Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
FfVideoState* Ffvs = instance->m_pVideoState;

while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
{
if (ExtClockStarted::EXT_CLOCK_STOP == Ffvs->m_sExtClock.is_start)
{
SDL_Delay(1);
continue;
}

if (PlayStatus::PLAY != Ffvs->m_iPlayingFlag || NetworkStatus::NETWORK_BUFFERING == Ffvs->m_iNetworkStatusFlag)
{
if (0 != Ffvs->m_sExtClock.timer)
{
if (SDL_bool::SDL_FALSE == SDL_RemoveTimer(Ffvs->m_sExtClock.timer))
{
Ffvs->m_iSynMethod = SynMethod::SYN_AUDIO;   //<! 如果计时器出现问题，切换同步方式 !>
PRINT_LOG("SDL timer error!\n");
break;
}
Ffvs->m_sExtClock.timer = 0;
}
continue;
}

if (0 == Ffvs->m_sExtClock.timer)
{
Ffvs->m_sExtClock.timer = SDL_AddTimer(FF_SDL_TIMER_INTERVAL, (instance->SDL_Timer_Cb), Ffvs);
}

}
PRINT_LOG("ExtClockThread returned!\n");
return RETURN_VALUE_SUCCESS;
}*/

void Ffmpeg::SetFilePath(std::string fn)
{
	m_sFilename = fn;
}

std::string Ffmpeg::GetFileName()
{
	return m_sFilename;
}

void Ffmpeg::SetHWND(HWND hwnd)
{
	m_hHwnd = hwnd;
}

HWND Ffmpeg::GetHWND()
{
	return m_hHwnd;
}

void Ffmpeg::SetWindowSize(uint32_t width, uint32_t height)
{
	m_iWndWidth = width;
	m_iWndHeight = height;

#if USE_SDL_VIDEO
	if (nullptr != m_pVideoState && nullptr != m_pVideoState->m_pScreen)
	{
		//SDL_SetWindowSize(m_pVideoState->m_pScreen, m_iWndWidth, m_iWndHeight);
	}
#endif
}


double Ffmpeg::SecondsToTicks(double sec)
{
	return sec * 1000 * 10000;
}

double Ffmpeg::TicksToSeconds(double ticks)
{
	return ticks / 1000 / 10000;
}

int32_t Ffmpeg::GetIsPlayFinished()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	return m_pVideoState->m_iPlayFinishedFlag;
}

int32_t Ffmpeg::GetPosition(uint64_t * position)
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	//double _last_position = m_pVideoState->m_dCurrentPosition;

	if (m_pVideoState->m_iSeekFlag != SeekStatus::SEEK_NOTHING)
	{
		m_pVideoState->m_dCurrentPosition = m_pVideoState->m_iSeekTime;
		*position = SecondsToTicks(m_pVideoState->m_dCurrentPosition);
		return  *position >= 0 ? S_OK : E_FAIL;
	}

	if (STREAM_TRACK_EMPTY == m_pVideoState->m_iAudioStream)  //<! 没有音频，使用视频时间戳作为当前时间进度显示 !>
	{
		m_pVideoState->m_dCurrentPosition = m_pVideoState->m_dCurrentVideoPts - m_pVideoState->m_sFirstFrame.FirstFramePts;
	}
	else
	{
		m_pVideoState->m_dCurrentPosition = m_pVideoState->m_dCurrentAudioPts - m_pVideoState->m_sFirstFrame.FirstFramePts;
	}

	//<! 若获取的pts大于获取的duration，已duration为准。 !>
	m_pVideoState->m_dCurrentPosition = m_pVideoState->m_dCurrentPosition > m_pVideoState->m_dFilmDurationInSecs ? m_pVideoState->m_dFilmDurationInSecs : m_pVideoState->m_dCurrentPosition;
	m_pVideoState->m_dCurrentPosition = m_pVideoState->m_dCurrentPosition < 0 ? 0 : m_pVideoState->m_dCurrentPosition;
	//PRINT_LOG("current time get :%f", m_pVideoState->m_dCurrentPosition);

	*position = SecondsToTicks(m_pVideoState->m_dCurrentPosition);
	return  *position >= 0 ? S_OK : E_FAIL;
}

int32_t Ffmpeg::VolumeUp(int32_t vo)
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	m_pVideoState->m_iCurrentVolume += vo;
	if (m_pVideoState->m_iCurrentVolume > 100)
	{
		m_pVideoState->m_iCurrentVolume = 100;
	}

	return m_pVideoState->m_iCurrentVolume;
}

int32_t Ffmpeg::VolumeDown(int32_t vo)
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	m_pVideoState->m_iCurrentVolume -= vo;
	if (m_pVideoState->m_iCurrentVolume < 0)
	{
		m_pVideoState->m_iCurrentVolume = 0;
	}

	return m_pVideoState->m_iCurrentVolume;
}

int32_t Ffmpeg::SetVolume(int32_t vo)
{
	return m_pVideoState->m_iCurrentVolume = vo;
}

uint32_t Ffmpeg::GetMovieWidth()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	return m_pVideoState->m_iFilmWidth;
}

uint32_t Ffmpeg::GetMovieHeight()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	return m_pVideoState->m_iFilmHeight;
}

int32_t Ffmpeg::GetFrame(SESSION* session, ID3D11Resource* resource)
{

	//if (session->mode == Video3DMode_Off)
	//{

	//	SDL_LockMutex(m_pVideoState->m_pGetFrameMutex);
	//	//deviceContextleft->UpdateSubresource(resource, 0, &destRegionleft, m_pVideoState->m_pVideoBufferForUnity, session->player->GetMovieWidth() << 2, 0);
	//	//if (FAILED(hr)) goto done;
	//	//m_pVideoState->m_pD11DeviceContext->Flush();

	//	m_pVideoState->m_pD11DeviceContextUnity->CopyResource(resource, m_pVideoState->m_pTmpTexture);

	//	SDL_UnlockMutex(m_pVideoState->m_pGetFrameMutex);
	//}


	return RETURN_VALUE_SUCCESS;
}


int32_t Ffmpeg::InitD3d11()
{


	//CHECK_PTR_RETURN_IF_NULL(leftEyeTexture, "leftEyeTexture is nullptr", RETURN_VALUE_FAILED);

	//ID3D11Texture2D* textureleft = NULL;
	//int hr = leftEyeTexture->QueryInterface(IID_ID3D11Texture2D, (void**)&(textureleft)); //<! COM查询Texture2D !>
	//if (FAILED(hr))
	//{
	//	return RETURN_VALUE_FAILED;
	//}
	//textureleft->GetDesc(&m_pVideoState->m_d311Texture2DDesc);  //<! 通过Texture2D查询Texture描述 !>
	//textureleft->GetDevice(&m_pVideoState->m_pD11DeviceUnity);  //<! 通过Texture2D查询设备 !>
	//m_pVideoState->m_pD11DeviceUnity->GetImmediateContext(&m_pVideoState->m_pD11DeviceContextUnity);//<! 通过Texture2D查询设备上下文 !>
	//SafeRelease(&textureleft);//<!释放临时texture2D

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = m_pVideoState->m_iFilmWidth;
	sd.BufferDesc.Height = m_pVideoState->m_iFilmHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = m_hHwnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;



	int hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&m_pVideoState->m_pD11SwapChain,
		&m_pVideoState->m_pD11Device,
		nullptr,
		&m_pVideoState->m_pD11DeviceContext);//<! 创建建一个新的共享设备，用来进行Texture异步拷贝操作 !>
	if (FAILED(hr))
	{
		DWORD err = GetLastError();
		return RETURN_VALUE_FAILED;
	}

	//m_pVideoState->m_d311Texture2DDesc.Width = m_pVideoState->m_iRenderTargetWidth / 2;
	//m_pVideoState->m_d311Texture2DDesc.Height = m_pVideoState->m_iRenderTargetHeight;
	//m_pVideoState->m_d311Texture2DDesc.ArraySize = 1;
	//m_pVideoState->m_d311Texture2DDesc.MipLevels = 1;
	//m_pVideoState->m_d311Texture2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	//m_pVideoState->m_d311Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	//m_pVideoState->m_d311Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//m_pVideoState->m_d311Texture2DDesc.SampleDesc.Count = 1;
	//m_pVideoState->m_d311Texture2DDesc.SampleDesc.Quality = 0;

	m_pVideoState->m_d311Texture2DDesc.Width = m_pVideoState->m_iFilmWidth / 2;
	m_pVideoState->m_d311Texture2DDesc.Height = m_pVideoState->m_iFilmHeight;
	m_pVideoState->m_d311Texture2DDesc.ArraySize = 1;
	m_pVideoState->m_d311Texture2DDesc.MipLevels = 1;
	m_pVideoState->m_d311Texture2DDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_pVideoState->m_d311Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	m_pVideoState->m_d311Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	m_pVideoState->m_d311Texture2DDesc.SampleDesc.Count = 1;
	//m_pVideoState->m_d311Texture2DDesc.SampleDesc.Quality = 0;
	//m_pVideoState->m_d311Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	m_pVideoState->m_d311Texture2DDesc.CPUAccessFlags = 0;




	//m_pVideoState->m_iRenderTargetWidth *= 2;


	if (m_pVideoState->m_iRenderTargetWidth < m_pVideoState->m_iFilmWidth || m_pVideoState->m_iRenderTargetHeight < m_pVideoState->m_iFilmHeight)
	{
		m_pVideoState->m_iRenderTargetWidth = m_pVideoState->m_iFilmWidth;
		m_pVideoState->m_iRenderTargetHeight = m_pVideoState->m_iFilmHeight;
		m_pVideoState->m_iIsScreenScale = 1;
		PicoPlugin::f_UE4InitWithRenderTargetSize(m_pVideoState->m_pD11Device, m_pVideoState->m_iRenderTargetWidth / 2, m_pVideoState->m_iRenderTargetHeight);

	}
	else
	{
		m_pVideoState->m_iIsScreenScale = 0;
		PicoPlugin::f_UE4Initialization(m_pVideoState->m_pD11Device);
	}


	D3D11_TEXTURE2D_DESC m_RenderTargetDesc;

	m_RenderTargetDesc.Width = m_pVideoState->m_iRenderTargetWidth / 2;
	m_RenderTargetDesc.Height = m_pVideoState->m_iRenderTargetHeight;
	m_RenderTargetDesc.ArraySize = 1;
	m_RenderTargetDesc.MipLevels = 1;
	m_RenderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_RenderTargetDesc.Usage = D3D11_USAGE_DEFAULT;
	m_RenderTargetDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	m_RenderTargetDesc.SampleDesc.Count = 1;
	m_RenderTargetDesc.SampleDesc.Quality = 0;
	m_RenderTargetDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	m_RenderTargetDesc.CPUAccessFlags = 0;

	for (int32_t nb = 0; nb < RENDER_TARGET_TEXTURE_NUM; ++nb)
	{
		m_pVideoState->m_pD11Device->CreateTexture2D(&m_RenderTargetDesc, NULL, &m_pVideoState->m_RenderTargetLeft[nb]);
		CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_RenderTargetLeft, "m_pVideoState->m_RenderTargetLeft is null!\n", RETURN_VALUE_FAILED);

		m_pVideoState->m_pD11Device->CreateTexture2D(&m_RenderTargetDesc, NULL, &m_pVideoState->m_RenderTargetRight[nb]);
		CHECK_PTR_RETURN_IF_NULL(m_pVideoState->m_RenderTargetRight, "m_pVideoState->m_RenderTargetRight is null!\n", RETURN_VALUE_FAILED);
	}


	return RETURN_VALUE_SUCCESS;

}

int32_t Ffmpeg::InitPicoHMD()
{

	if (!PicoPlugin::Initialize())
	{
		return RETURN_VALUE_FAILED;
	}

	//if (!PicoPlugin::f_PVR_InitWithRenderTargetSize(m_pVideoState->m_iRenderTargetWidth / 2, m_pVideoState->m_iRenderTargetHeight))
	//{
	//	return RETURN_VALUE_FAILED;
	//}

	if (!PicoPlugin::f_PVR_Init())
	{
		return RETURN_VALUE_FAILED;
	}

	PicoPlugin::f_PVR_GetRenderTargetSize(m_pVideoState->m_iRenderTargetWidth, m_pVideoState->m_iRenderTargetHeight);


	//PicoPlugin::f_UE4Initialization(m_pVideoState->m_pD11Device);


	return RETURN_VALUE_SUCCESS;
}


int32_t Ffmpeg::ReleasePicoHMD()
{
	if (PicoPlugin::GetIsInitialized())
	{
		PicoPlugin::f_UE4Shutdown();
		PicoPlugin::f_PVR_Shutdown();
		Sleep(3000);
	}
	return RETURN_VALUE_SUCCESS;
}



int Ffmpeg::AudioThread(void* ptr)
{
	Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;


	SDL_PauseAudio(0);

	while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
	{

		//PRINT_LOG("AudioThread\n");
		if (Ffvs->m_iPlayingFlag == PlayStatus::PAUSE)
		{

			SDL_Delay(1);
			continue;
		}
		else if (Ffvs->m_iPlayingFlag == PlayStatus::PLAY)
		{
			//if (Ffvs->m_dCurrentAudioPts >= Ffvs->m_dCurrentVideoPts + AUDIO_VIDEO_DIFFER_TIME)
			//{
			//	if (Ffvs->m_VideoQueue->Length > 0 && Ffvs->m_AudioQueue->Length < AudioBufferMaxSize)
			//	{
			//		//PRINT_LOG("Audio Fast!\n");
			//		SDL_Delay(1);
			//		continue;
			//	}
			//}
			uint8_t* mixed_buffer = nullptr;
			AudioQueueItem* audioItem = nullptr;
			//从队列中拿出音频数据
			if (instance->GetItemFromAudioQueue(Ffvs->m_AudioQueue, &audioItem))
			{

				if (Ffvs->m_iSeekFlag != SeekStatus::SEEK_NOTHING)   //<! seek 过程中取出的item丢弃 !>
				{
					goto free;
				}


				//SDL_PauseAudio(0);

				//Ffvs->audio_len = audioItem->Length;
				//Ffvs->audio_pos = audioItem->AudioData;
				Ffvs->m_dCurrentAudioPts = audioItem->Pts;//当前音频时间戳

				mixed_buffer = (uint8_t*)av_malloc(audioItem->Length);
				if (nullptr == mixed_buffer)
				{
					PRINT_LOG("mixed_buffer is null! no audio output");
					goto free;
				}
				SDL_memset(mixed_buffer, 0, audioItem->Length);
				SDL_MixAudio(mixed_buffer, audioItem->AudioData, audioItem->Length, Ffvs->m_iCurrentVolume * 1.28);


				SDL_PauseAudio(1);
				int32_t i = SDL_QueueAudio((SDL_AudioDeviceID)1, mixed_buffer, audioItem->Length);
				SDL_PauseAudio(0);
				while (SDL_GetQueuedAudioSize((SDL_AudioDeviceID)1) != 0)
				{
					SDL_Delay(1);
				}

				//while (Ffvs->audio_len > 0 && Ffvs->m_iPlayingFlag != STOP)   //Wait until play finished  //如果停止，则不需要等待,因为当STOP时，AudioCallback已经停止。
				//{
				//	SDL_Delay(1);
				//}
				//SDL_PauseAudio(1);

			}
		free:
			if (nullptr != audioItem)
			{
				av_free(audioItem->AudioData);
				av_free(audioItem);
			}
			if (nullptr != mixed_buffer)
			{
				av_free(mixed_buffer);
			}
		}
	}

	SDL_PauseAudio(1);
	SDL_CondSignal(Ffvs->m_AudioQueue->audioCond);
	PRINT_LOG("AudioThread Returned.\n");

	return RETURN_VALUE_SUCCESS;
}


void Ffmpeg::SubmitToHMD(ID3D11Texture2D* leftTex, ID3D11Texture2D* rightTex, D3D11_BOX* texCopyRegion, int32_t Scale)
{
	static int32_t index = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	RenderTexture rt;
	pvrQuaternion_t quat_double;

	if (!PicoPlugin::f_PVR_GetTrackedPose(quat_double))
	{
		return;
	}

	rt.orientation.w = quat_double.w;
	rt.orientation.x = -quat_double.x;
	rt.orientation.y = -quat_double.y;
	rt.orientation.z = quat_double.z;

	if (!Scale)
	{
		int32_t _index = index++ % RENDER_TARGET_TEXTURE_NUM;
		index = index == RENDER_TARGET_TEXTURE_NUM ? 0 : index;

		//PRINT_LOG("%d",_index);
		x = m_pVideoState->m_iRenderTargetWidth > m_pVideoState->m_iFilmWidth ? (m_pVideoState->m_iRenderTargetWidth - m_pVideoState->m_iFilmWidth) / 4 : 0;
		y = m_pVideoState->m_iRenderTargetHeight > m_pVideoState->m_iFilmHeight ? (m_pVideoState->m_iRenderTargetHeight - m_pVideoState->m_iFilmHeight) / 2 : 0;


		m_pVideoState->m_pD11DeviceContext->CopySubresourceRegion(m_pVideoState->m_RenderTargetLeft[_index], 0, x, y, 0, leftTex, 0, texCopyRegion);
		m_pVideoState->m_pD11DeviceContext->CopySubresourceRegion(m_pVideoState->m_RenderTargetRight[_index], 0, x, y, 0, rightTex, 0, texCopyRegion);

		rt.LRT = m_pVideoState->m_RenderTargetLeft[_index];
		rt.RRT = m_pVideoState->m_RenderTargetRight[_index];
	}
	else
	{
		rt.LRT = leftTex;
		rt.RRT = rightTex;
	}


	PicoPlugin::f_SetTextureFromUnityATW(rt);

}



int Ffmpeg::VideoThread(void* ptr)
{
	Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;

	int32_t _diff_error_count = 0;

	while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
	{
		//PRINT_LOG("VideoThread\n");
		if (Ffvs->m_iPlayingFlag == PlayStatus::PAUSE)
		{
			SDL_Delay(1);
			continue;
		}
		else if (Ffvs->m_iPlayingFlag == PlayStatus::PLAY)
		{

			if (SynMethod::SYN_MULTI == Ffvs->m_iSynMethod)
			{

				int32_t _delay = instance->GetDelay(Ffvs);

				SDL_Delay(_delay);

			}
			else
			{
				SDL_Delay(Ffvs->m_iFrameDelay);
			}

			VideoQueueItem* videoItem = nullptr;
			//从队列中拿出视频数据
			//PRINT_LOG("Get Video Item Start\n");
			if (instance->GetItemFromVideoQueue(Ffvs->m_VideoQueue, &videoItem))
			{

				if (Ffvs->m_iSeekFlag != SeekStatus::SEEK_NOTHING)   //<! seek 过程中取出的item丢弃 !>
				{
					goto free;
				}

				//PRINT_LOG("Get Video Item End\n");
				double _lastVideoPts = 0;
				double _diff = 0;


				_lastVideoPts = Ffvs->m_dCurrentVideoPts;
				Ffvs->m_dCurrentVideoPts = videoItem->Pts;//当前视频时间戳
				_diff = Ffvs->m_dCurrentVideoPts - _lastVideoPts;//<! 计算视频间隔 !>
				if (_diff <= 0 || _diff >= MAX_VIDEO_DIFFER_TIME)
				{
					_diff_error_count += 1;
					if (_diff_error_count > MAX_VIDEO_DIFFER_ERROR)
					{
						Ffvs->m_iFrameDelay = (Ffvs->m_iFPS == 0 ? (double)1000 / (double)30 : (double)1000 / (double)Ffvs->m_iFPS);  //<! 若获取不到任何播放时间信息，选取默认30fps !>
						Ffvs->m_iSynMethod = SynMethod::SYN_FRAMERATE;
						PRINT_LOG("Syn method switched!\n");
					}
					goto free;
				}
				else
				{
					Ffvs->m_iFrameDelay = (_diff * 1000 - 1) < 0 ? Ffvs->m_iFrameDelay : (_diff * 1000 - 1);
					_diff_error_count = 0;
				}

#if USE_SDL_VIDEO
				if (nullptr != videoItem->Texture->raw_pixs)
				{
					SDL_LockMutex(Ffvs->m_pGetFrameMutex);
					//SDL_memcpy4()
					int64_t begin_time = av_gettime_relative();
					SDL_memcpy(Ffvs->m_pSDLVideoBuffer, videoItem->Texture->raw_pixs, Ffvs->m_iBufferSizeInBytes);
					Ffvs->m_iMemcpyCostTime = av_gettime_relative() - begin_time;
					SDL_UnlockMutex(Ffvs->m_pGetFrameMutex);
				}
#else
				D3D11_BOX destRegion;
				destRegion.left = 0;
				destRegion.top = 0;
				destRegion.right = Ffvs->m_iFilmWidth / 2;
				destRegion.bottom = Ffvs->m_iFilmHeight;
				destRegion.front = 0;
				destRegion.back = 1;

				ID3D11Texture2D * p_RT;
				Ffvs->m_pD11SwapChain->GetBuffer(0, __uuidof(p_RT), reinterpret_cast<void**>(&p_RT));
				Ffvs->m_pD11DeviceContext->CopySubresourceRegion(p_RT, 0, 0, 0, 0, reinterpret_cast<ID3D11Texture2D *>(videoItem->Texture->rt->LRT), 0, &destRegion);
				Ffvs->m_pD11DeviceContext->CopySubresourceRegion(p_RT, 0, Ffvs->m_iFilmWidth / 2, 0, 0, reinterpret_cast<ID3D11Texture2D *>(videoItem->Texture->rt->RRT), 0, &destRegion);
				Ffvs->m_pD11SwapChain->Present(0, 0);

				//ID3D11Resource* SaveTextureResource;
				//HRESULT hr;

				//static int ii = 0;

				//if (ii++ % 60)
				//{
				//	wchar_t path[MAX_PATH];

				//	swprintf(path, L"D:\\Test%d.jpg", ii);

				//	//std::wstring path1 = L"D:\\Test" + _itow(ii);

				//	hr = ((ID3D11Texture2D*)(videoItem->Texture->rt->LRT))->QueryInterface(__uuidof(ID3D11Resource), (void**)&SaveTextureResource);
				//	hr = D3DX11SaveTextureToFile(Ffvs->m_pD11DeviceContext, SaveTextureResource, D3DX11_IFF_JPG, path);
				//	DWORD err = GetLastError();
				//}

				//PRINT_LOG("LRT:%ld,RRT:%ld", videoItem->Texture->rt->LRT, videoItem->Texture->rt->RRT);
#endif
				//pvrQuaternion_t quat_double;
				//if (!PicoPlugin::f_PVR_GetTrackedPose(quat_double))
				//{
				//	goto free;
				//}

				//videoItem->Texture->rt->orientation.w = quat_double.w;
				//videoItem->Texture->rt->orientation.x = -quat_double.x;
				//videoItem->Texture->rt->orientation.y = -quat_double.y;
				//videoItem->Texture->rt->orientation.z = quat_double.z;

				//PicoPlugin::f_SetTextureFromUnityATW(*(videoItem->Texture->rt));
				instance->SubmitToHMD(reinterpret_cast<ID3D11Texture2D *>(videoItem->Texture->rt->LRT), reinterpret_cast<ID3D11Texture2D *>(videoItem->Texture->rt->RRT), &destRegion, Ffvs->m_iIsScreenScale);
				}

		free:
			if (nullptr != videoItem)
			{
#if USE_SDL_VIDEO
				av_free(videoItem->Texture->raw_pixs);
#endif
				//videoItem->Texture->raw_pixs = nullptr;
				instance->SetGPUMemEnable(videoItem->Texture);
				av_free(videoItem);
			}
		}
	}

	SDL_CondSignal(Ffvs->m_VideoQueue->videoCond);

	PRINT_LOG("VideoThread Returned.\n");

	return RETURN_VALUE_SUCCESS;
			}


#if USE_SDL_VIDEO

int Ffmpeg::SDLVideoThread(void* ptr)
{

	Ffmpeg* instance = reinterpret_cast<Ffmpeg*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;

	while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
	{
		//PRINT_LOG("VideoThread\n");
		if (Ffvs->m_iPlayingFlag == PlayStatus::PAUSE)
		{
			SDL_Delay(1);
			continue;
		}
		else if (Ffvs->m_iPlayingFlag == PlayStatus::PLAY)
		{

			//SDL---------------------------

			int32_t ret;
			SDL_LockMutex(Ffvs->m_pGetFrameMutex);

			//int64_t begin_time = av_gettime_relative();
			ret = SDL_UpdateTexture(Ffvs->m_pSDLTexture, &Ffvs->m_rect, Ffvs->m_pSDLVideoBuffer, Ffvs->m_pFrameYUV->linesize[0]);
			//PRINT_LOG("SDL_UpdateTexture cost time : %ld", av_gettime_relative() - begin_time);

			SDL_UnlockMutex(Ffvs->m_pGetFrameMutex);


			if (ret == 0)
			{
				ret = SDL_RenderClear(Ffvs->m_pRender);
				SDL_Rect rect = instance->RecalculateWidthHeightRatio(Ffvs);//获取显示尺寸
				ret = SDL_RenderCopy(Ffvs->m_pRender, Ffvs->m_pSDLTexture, &Ffvs->m_rect, &rect);
				SDL_RenderPresent(Ffvs->m_pRender);
			}
			//SDL End-----------------------

		}
	}

	PRINT_LOG("SDLVideoThread Returned.\n");

	return RETURN_VALUE_SUCCESS;
}
#endif

int32_t Ffmpeg::InitGPUBuffers()
{
	if (nullptr == m_pVideoState->m_pGpuMemMap)
	{
		m_pVideoState->m_pGpuMemMap = new std::map<PictureTexture*, int32_t>;   //<! 创建一个显存地址Map，用来管理显存 !>
}

	for (uint32_t ii = 0; ii < MAX_GPU_BUFFERS; ii++)//<! 创建显存队列，用来进行缓冲 !>
	{

		ID3D11Texture2D* _textureLeft = nullptr;
		ID3D11Texture2D* _textureRight = nullptr;
		PictureTexture* _pixtureTexture = nullptr;

		_pixtureTexture = (PictureTexture*)av_mallocz(sizeof(PictureTexture));
		_pixtureTexture->rt = (RenderTexture*)av_mallocz(sizeof(RenderTexture));


		m_pVideoState->m_pD11Device->CreateTexture2D(&m_pVideoState->m_d311Texture2DDesc, NULL, &_textureLeft);
		m_pVideoState->m_pD11Device->CreateTexture2D(&m_pVideoState->m_d311Texture2DDesc, NULL, &_textureRight);

		//_pix = (uint8_t*)av_mallocz(m_pVideoState->m_iBufferSizeInBytes);  //<!-- 连续申请大内存可能失败，改为动态内存申请 -->


		if (_textureLeft == nullptr ||
			_textureRight == nullptr ||
			_pixtureTexture == nullptr)
		{
			//return RETURN_VALUE_FAILED;

			PRINT_LOG("Buffer allocate failed, %d", ii);
			break;
		}


		_pixtureTexture->rt->LRT = _textureLeft;
		_pixtureTexture->rt->RRT = _textureRight;

		m_pVideoState->m_pGpuMemMap->insert(std::pair<PictureTexture*, int32_t>(_pixtureTexture, GpuMemFlag::GPU_MEM_ENABLE));
	}

	return RETURN_VALUE_SUCCESS;
		}

int32_t Ffmpeg::ReleaseD3D11()
{

	if (nullptr != m_pVideoState->m_pD11Device)
	{
		SafeRelease(&m_pVideoState->m_pD11Device);
	}
	if (nullptr != m_pVideoState->m_pD11DeviceContext)
	{
		SafeRelease(&m_pVideoState->m_pD11DeviceContext);
	}

	//if (nullptr != m_pVideoState->m_pGpuMemMap)
	//{
	//	for (std::map<PictureTexture *, int32_t>::iterator _it = m_pVideoState->m_pGpuMemMap->begin(); _it != m_pVideoState->m_pGpuMemMap->end(); _it++)
	//	{
	//		//_it->first->Release();
	//	}

	//	delete m_pVideoState->m_pGpuMemMap;

	//}

	//if (nullptr != m_pVideoState->m_pGPUTempBuffer)
	//{
	//	SafeRelease(&m_pVideoState->m_pGPUTempBuffer);
	//}
	//if (nullptr != m_pVideoState->m_pTmpResource)
	//{
	//	SafeRelease(&m_pVideoState->m_pTmpResource);
	//}
	//if (nullptr != m_pVideoState->m_pTmpTexture)
	//{
	//	SafeRelease(&m_pVideoState->m_pTmpTexture);
	//}

	return RETURN_VALUE_SUCCESS;
}

int32_t Ffmpeg::ReleaseGPUBuffers()
{
	if (nullptr != m_pVideoState->m_pGpuMemMap)
	{
		for (std::map<PictureTexture *, int32_t>::iterator _it = m_pVideoState->m_pGpuMemMap->begin(); _it != m_pVideoState->m_pGpuMemMap->end(); _it++)
		{
			SafeRelease((ID3D11Texture2D**)&(_it->first->rt->LRT));
			SafeRelease((ID3D11Texture2D**)&(_it->first->rt->RRT));
			av_free(_it->first->rt);
			_it->first->rt = NULL;
			//av_free(_it->first->raw_pixs);
			//_it->first->raw_pixs = NULL;
		}

		delete m_pVideoState->m_pGpuMemMap;
	}

	return RETURN_VALUE_SUCCESS;
}


PictureTexture* Ffmpeg::FindEnabledGPUMem()
{

	//PRINT_LOG("FindEnabledGPUMem");

	SDL_LockMutex(m_pVideoState->m_pGpuMemMapMutex);

	PictureTexture* _pRetPointer = nullptr;
	for (std::map<PictureTexture *, int32_t>::iterator _it = m_pVideoState->m_pGpuMemMap->begin(); _it != m_pVideoState->m_pGpuMemMap->end(); _it++)
	{
		if (GpuMemFlag::GPU_MEM_ENABLE == _it->second)
		{
			_it->second = GpuMemFlag::GPU_MEM_DISABLE;
			_pRetPointer = _it->first;
			break;
		}
	}

	SDL_UnlockMutex(m_pVideoState->m_pGpuMemMapMutex);

	return _pRetPointer;
}

int32_t Ffmpeg::SetGPUMemEnable(PictureTexture* texturePointer)
{

	SDL_LockMutex(m_pVideoState->m_pGpuMemMapMutex);

	(*(m_pVideoState->m_pGpuMemMap))[texturePointer] = GpuMemFlag::GPU_MEM_ENABLE;

	SDL_UnlockMutex(m_pVideoState->m_pGpuMemMapMutex);

	return RETURN_VALUE_SUCCESS;
}

uint32_t Ffmpeg::GetLastFrameCount()
{
	return m_pVideoState->m_iLastFrameUploaded;
}

int32_t Ffmpeg::Pause()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	return m_pVideoState->m_iPlayingFlag = PlayStatus::PAUSE;
}

//int32_t Ffmpeg::LockAudio()     //<! 暂停声音callback，否则缓冲或者暂停时，某些视频有杂音 !>
//{
//	if (AudioCBLockStatus::AUDIO_CB_UNLOCKED ==  m_pVideoState->m_eAudioCBStatus)
//	{
//		SDL_LockAudio();
//		m_pVideoState->m_eAudioCBStatus = AudioCBLockStatus::AUDIO_CB_LOCKED;
//	}
//	return m_pVideoState->m_eAudioCBStatus;
//}
//
//int32_t Ffmpeg::UnlockAudio()
//{
//	if (AudioCBLockStatus::AUDIO_CB_LOCKED == m_pVideoState->m_eAudioCBStatus)
//	{
//		SDL_UnlockAudio();
//		m_pVideoState->m_eAudioCBStatus = AudioCBLockStatus::AUDIO_CB_UNLOCKED;
//	}
//	return m_pVideoState->m_eAudioCBStatus;
//}


