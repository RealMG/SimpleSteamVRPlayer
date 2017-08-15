
#include "ffmpeg_local.h"

FfmpegLocal::FfmpegLocal()
{

}

FfmpegLocal::~FfmpegLocal()
{

}
/*

int FfmpegLocal::AudioThread(void* ptr)
{
	FfmpegLocal* instance = reinterpret_cast<FfmpegLocal*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;


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

			uint8_t* mixed_buffer = nullptr;
			AudioQueueItem* audioItem = nullptr;
			//�Ӷ������ó���Ƶ����
			//PRINT_LOG("Get Audio Item Start\n");
			if (instance->GetItemFromAudioQueue(Ffvs->m_AudioQueue, &audioItem))
			{

				if (Ffvs->m_iSeekFlag != SeekStatus::SEEK_NOTHING)   //<! seek ������ȡ����item���� !>
				{
					goto free;
				}
				//PRINT_LOG("Get Audio Item End\n");

				Ffvs->m_dCurrentAudioPts = audioItem->Pts;//��ǰ��Ƶʱ���

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

	SDL_CondSignal(Ffvs->m_AudioQueue->audioCond);
	PRINT_LOG("AudioThread Returned.\n");

	return RETURN_VALUE_SUCCESS;
}

int FfmpegLocal::VideoThread(void* ptr)
{
	FfmpegLocal* instance = reinterpret_cast<FfmpegLocal*>(ptr);
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
				//PRINT_LOG("Current Delay: %d", _delay);
				SDL_Delay(_delay);

			}
			else
			{
				SDL_Delay(Ffvs->m_iFrameDelay);
			}

			VideoQueueItem* videoItem = nullptr;
			//�Ӷ������ó���Ƶ����
			//PRINT_LOG("Get Video Item Start\n");
			if (instance->GetItemFromVideoQueue(Ffvs->m_VideoQueue, &videoItem))
			{
				if (Ffvs->m_iSeekFlag != SeekStatus::SEEK_NOTHING)   //<! seek ������ȡ����item���� !>
				{
					goto free;
				}

				//PRINT_LOG("Get Video Item End\n");
				double _lastVideoPts = 0;
				double _diff = 0;

				_lastVideoPts = Ffvs->m_dCurrentVideoPts;
				Ffvs->m_dCurrentVideoPts = videoItem->Pts;//��ǰ��Ƶʱ���
				_diff = Ffvs->m_dCurrentVideoPts - _lastVideoPts;//<! ������Ƶ��� !>
				if (_diff <= 0 || _diff >= MAX_VIDEO_DIFFER_TIME)
				{
					_diff_error_count += 1;
					if (_diff_error_count > MAX_VIDEO_DIFFER_ERROR)
					{
						Ffvs->m_iFrameDelay = (Ffvs->m_iFPS == 0 ? (double)1000 / (double)30 : (double)1000 / (double)Ffvs->m_iFPS);  //<! ����ȡ�����κβ���ʱ����Ϣ��ѡȡĬ��30fps !>
						Ffvs->m_iSynMethod = SynMethod::SYN_FRAMERATE;
						PRINT_LOG("Syn method switched!\n");
					}
					goto free;   //<! �������̫�󣬶�֡ !>
				}
				else
				{
					Ffvs->m_iFrameDelay = (_diff * 1000 - 1) < 0 ? Ffvs->m_iFrameDelay : (_diff * 1000 - 1);   //<! ����1ms��ʱ�� !>
					_diff_error_count = 0;
					Ffvs->m_iSynMethod = Ffvs->m_iFirstSynMethodSelected;   //<! ���ʱ����ָ���������ָ�ԭ����ͬ����ʽ !>
				}

				//SDL---------------------------
#if USE_SDL_VIDEO


				//int64_t begin_time = av_gettime_relative();
				int ret = SDL_UpdateTexture(Ffvs->m_pSDLTexture, &Ffvs->m_rect, videoItem->Texture->raw_pixs, Ffvs->m_pFrameYUV->linesize[0]);
				//PRINT_LOG("sdl update time : %ld", av_gettime_relative() - begin_time);

				//PRINT_LOG("SDL display start");
				ret = SDL_RenderClear(Ffvs->m_pRender);
				SDL_Rect rect = instance->RecalculateWidthHeightRatio(Ffvs);//��ȡ��ʾ�ߴ�

				ret = SDL_RenderCopy(Ffvs->m_pRender, Ffvs->m_pSDLTexture, &Ffvs->m_rect, &rect);
				SDL_RenderPresent(Ffvs->m_pRender);
				//PRINT_LOG("sdl  render cost time : %ld", av_gettime_relative() - begin_time);
#endif
				//SDL End-----------------------


				pvrQuaternion_t quat_double;
				if (!PicoPlugin::f_PVR_GetTrackedPose(quat_double))
				{
					goto free;
				}
				videoItem->Texture->rt->orientation.w = quat_double.w;
				videoItem->Texture->rt->orientation.x = -quat_double.x;
				videoItem->Texture->rt->orientation.y = -quat_double.y;
				videoItem->Texture->rt->orientation.z = quat_double.z;


				//static int ii = 0;

				//if (0x8000 & GetAsyncKeyState('P'))
				//{
				//	//PRINT_LOG("start %ld", videoItem->Texture->rt->LRT);
				//	ID3D11Resource* SaveTextureResource;
				//	HRESULT hr;

				//	wchar_t path[MAX_PATH];
				//	swprintf(path, L"D:\\Test%d.jpg", ii++);

				//	wchar_t path1[MAX_PATH];
				//	swprintf(path1, L"D:\\Test%d.jpg", ii++);
				//	//std::wstring path1 = L"D:\\Test" + _itow(ii);

				//	hr = ((ID3D11Texture2D*)(videoItem->Texture->rt->LRT))->QueryInterface(__uuidof(ID3D11Resource), (void**)&SaveTextureResource);
				//	hr = D3DX11SaveTextureToFile(Ffvs->m_pD11DeviceContext, SaveTextureResource, D3DX11_IFF_JPG, path);

				//	hr = ((ID3D11Texture2D*)(videoItem->Texture->rt->RRT))->QueryInterface(__uuidof(ID3D11Resource), (void**)&SaveTextureResource);
				//	hr = D3DX11SaveTextureToFile(Ffvs->m_pD11DeviceContext, SaveTextureResource, D3DX11_IFF_JPG, path1);

				//	DWORD err = GetLastError();

				//	//PRINT_LOG("end %ld", videoItem->Texture->rt->LRT);
				//}
				PicoPlugin::f_SetTextureFromUnityATW(*(videoItem->Texture->rt));

			}

		free:
			if (nullptr != videoItem)
			{
				//av_free(videoItem->Texture->raw_pixs);
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

*/
int FfmpegLocal::ReadThread(void* ptr)
{
	//!<start,  play the video.   $Nico@2016-11-2 09:42:59
	//!<
	//!<
	FfmpegLocal* instance = reinterpret_cast<FfmpegLocal*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;
	int32_t isEnd = 0;

	while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
	{

		if (Ffvs->m_iSeekFlag == SeekStatus::SEEK_TIME)
		{

			Ffvs->m_dCurrentAudioPts = Ffvs->m_iSeekTime;   //<! ��seekʱ�丳ֵ����ǰʱ�䣬����������ʾ !>
			Ffvs->m_dCurrentVideoPts = Ffvs->m_iSeekTime;   //<! ��seekʱ�丳ֵ����ǰʱ�䣬����������ʾ !>
			//double _seek_offset = 0;
			//if (STREAM_TRACK_EMPTY == Ffvs->m_iAudioStream)  //<! ĳЩ��Ƶ ����m3u8����һ֡��ʱ�䲻��0�������Ҫƫ�� !>
			//{
			//	_seek_offset = Ffvs->m_sFirstVideoFrame.FirstFramePts;
			//}
			//else
			//{
			//	_seek_offset = Ffvs->m_sFirstAudioFrame.FirstFramePts;
			//}


			int32_t ret = av_seek_frame(
				Ffvs->m_pFormatCtx,
				-1,
				(Ffvs->m_iSeekTime) * AV_TIME_BASE,
				AVSEEK_FLAG_BACKWARD
			);

			if (ret >= 0 && Ffvs->m_iBufferCleanFlag == BufferCleanFlag::BUFFER_CLEAN)  //<! ��ǰ������ת��������档 !>
			{
				instance->ClearVideoQueue(Ffvs->m_VideoQueue);
				instance->ClearAudioQueue(Ffvs->m_AudioQueue);
				avio_flush(Ffvs->m_pFormatCtx->pb);
				avformat_flush(Ffvs->m_pFormatCtx);
				//Ffvs->m_iPacketThrowCount = PACKET_THROW_COUNT_AFTER_SEEK;   //<! ÿ��seek�꣬��Ҫ����ǰ��֡ !>
			}

			Ffvs->m_iSeekFlag = SeekStatus::SEEK_NOTHING;

			continue;
		}

		if (Ffvs->m_iPlayingFlag == PlayStatus::PAUSE)
		{

			SDL_Delay(1);
			continue;
		}

		isEnd = av_read_frame(Ffvs->m_pFormatCtx, Ffvs->m_pflush_pkt);


		if (isEnd < 0 /*|| Ffvs->m_iPacketThrowCount > 0*/)
		{
			//if (Ffvs->m_iPacketThrowCount > 0)
			//{
			//	Ffvs->m_iPacketThrowCount--;
			//}
			if (AVERROR_EOF == isEnd)    //<! ��ȡ���ļ�ĩβ�����൱�ڲ��ŵ��ļ�ĩβ !>
			{
				Ffvs->m_iPlayFinishedFlag = PLAY_FINISHED;
			}
		}
		else
		{
			if (Ffvs->m_pflush_pkt->stream_index == Ffvs->m_iVideoStream)
			{
				instance->CreateVideoItemFromPacket(Ffvs->m_pflush_pkt);
			}
			if (Ffvs->m_pflush_pkt->stream_index == Ffvs->m_iAudioStream)
			{
				instance->CreateAudioItemFromPacket(Ffvs->m_pflush_pkt);
			}
		}
		av_packet_unref(Ffvs->m_pflush_pkt);

	}
	//!<end,  play the video.   $Nico@2016-11-2 09:42:59
	//!<
	//!<

	SDL_CondSignal(Ffvs->m_AudioQueue->audioCond);
	SDL_CondSignal(Ffvs->m_VideoQueue->videoCond);
#if USE_SDL_VIDEO
	if (nullptr != instance->m_pSDLVideoThread)
	{
		SDL_WaitThread(instance->m_pSDLVideoThread, NULL);
	}
#endif
	if (nullptr != instance->m_pAudioThread)
	{
		SDL_WaitThread(instance->m_pAudioThread, NULL);
	}
	if (nullptr != instance->m_pVideoThread)
	{
		SDL_WaitThread(instance->m_pVideoThread, NULL);
	}

	instance->ReleaseSources();
	//!<
	PRINT_LOG("ReadThread Returned.\n");

	return RETURN_VALUE_SUCCESS;
}

int32_t FfmpegLocal::InitThread()
{
	this->m_pReadThread = SDL_CreateThread(&FfmpegLocal::ReadThread, "ReadThread", this);  //<! ��ȡ�߳�
	CHECK_PTR_RETURN_IF_NULL(this->m_pReadThread, "ReadThread null ptr! \n", RETURN_VALUE_FAILED);

	if (m_pVideoState->m_iAudioStream != STREAM_TRACK_EMPTY) //<! ������Ƶʱ���������򲻴��� !>
	{
		this->m_pAudioThread = SDL_CreateThread(&FfmpegLocal::AudioThread, "AudioThread", this); //!< ��Ƶ�����߳�
		CHECK_PTR_RETURN_IF_NULL(this->m_pAudioThread, "AudioThread null ptr! \n", RETURN_VALUE_FAILED);
	}

	if (m_pVideoState->m_iVideoStream != STREAM_TRACK_EMPTY)//<! ������Ƶʱ���������򲻴��� !>
	{
		this->m_pVideoThread = SDL_CreateThread(&FfmpegLocal::VideoThread, "VideoThread", this); //<! ��Ƶ�����߳�
		CHECK_PTR_RETURN_IF_NULL(this->m_pVideoThread, "VideoThread null ptr! \n", RETURN_VALUE_FAILED);
		//this->m_pExtClockThread = SDL_CreateThread(&FfmpegLocal::ExtClockThread, "ExtClockThread", this); //<! �ⲿʱ���߳���Ϊ��Ƶͬ��Դ
	}

#if USE_SDL_VIDEO
	this->m_pSDLVideoThread = SDL_CreateThread(&FfmpegLocal::SDLVideoThread, "SDLVideoThread", this); //<! ��Ƶ�����߳�
	CHECK_PTR_RETURN_IF_NULL(this->m_pVideoThread, "SDLVideoThread null ptr! \n", RETURN_VALUE_FAILED);
#endif // 




	PRINT_LOG("Init Thread ok!\n");
	return RETURN_VALUE_SUCCESS;
}

