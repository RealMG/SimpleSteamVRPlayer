#include "ffmpeg_online.h"

#include "Shellapi.h"
#include "wchar.h"
#include <math.h> 

FfmpegOnline::FfmpegOnline() :
	m_pCacheThread(nullptr),
	m_CacheQueue(nullptr),
	m_ui64CacheSpaceInKB(NULL),
	m_ui64CurrentCacheSpaceInKB(NULL)
{

}

FfmpegOnline::~FfmpegOnline()
{

}

int32_t FfmpegOnline::Open(std::string fileName) //<! 重写父类的Open函数， 为了获取缓存目录
{

	if (RETURN_VALUE_SUCCESS != CreateTempFoldFromSystemTempPath(this->m_lSystemTempPath, MAX_PATH))
	{
		return RETURN_VALUE_FAILED;
	}


	if (RETURN_VALUE_SUCCESS != Ffmpeg::Open(fileName))
	{
		return RETURN_VALUE_FAILED;
	}


	return RETURN_VALUE_SUCCESS;
}

int32_t FfmpegOnline::Stop()
{

	if (RETURN_VALUE_SUCCESS != Ffmpeg::Stop())
	{
		return RETURN_VALUE_FAILED;
	}

	return RETURN_VALUE_SUCCESS;
}


int32_t FfmpegOnline::GetCachedPosition(uint64_t * position)
{

	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	if (STREAM_TRACK_EMPTY == m_pVideoState->m_iAudioStream)
	{
		m_pVideoState->m_dCurrentCachePosition = m_pVideoState->m_dCurrentCacheVideoPts - m_pVideoState->m_sFirstFrame.FirstFramePts;
	}
	else
	{
		m_pVideoState->m_dCurrentCachePosition = m_pVideoState->m_dCurrentCacheAudioPts - m_pVideoState->m_sFirstFrame.FirstFramePts;
	}

	m_pVideoState->m_dCurrentCachePosition = m_pVideoState->m_dCurrentCachePosition < 0 ? 0 : m_pVideoState->m_dCurrentCachePosition;

	*position = SecondsToTicks(m_pVideoState->m_dCurrentCachePosition);
	return  *position >= 0 ? S_OK : E_FAIL;

}

int32_t FfmpegOnline::GetIsBuffering()
{
	CHECK_PTR_RETURN_IF_NULL(m_pVideoState, "m_pVideoState is null!\n", RETURN_VALUE_FAILED);

	return m_pVideoState->m_iNetworkBufferingStatusFlag;
}

/*
int FfmpegOnline::AudioThread(void* ptr)
{
	FfmpegOnline* instance = reinterpret_cast<FfmpegOnline*>(ptr);
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

int FfmpegOnline::VideoThread(void* ptr)
{
	FfmpegOnline* instance = reinterpret_cast<FfmpegOnline*>(ptr);
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

				//SDL---------------------------
#if USE_SDL_VIDEO

				//int64_t begin_time = av_gettime_relative();
				int ret = SDL_UpdateTexture(Ffvs->m_pSDLTexture, &Ffvs->m_rect, videoItem->Texture->raw_pixs, Ffvs->m_pFrameYUV->linesize[0]);
				//PRINT_LOG("sdl update time : %ld", av_gettime_relative() - begin_time);

				//PRINT_LOG("SDL display start");
				ret = SDL_RenderClear(Ffvs->m_pRender);
				SDL_Rect rect = instance->RecalculateWidthHeightRatio(Ffvs);//获取显示尺寸

				ret = SDL_RenderCopy(Ffvs->m_pRender, Ffvs->m_pSDLTexture, &Ffvs->m_rect, &rect);
				SDL_RenderPresent(Ffvs->m_pRender);
				//PRINT_LOG("SDL display end");
#endif
				//SDL End-----------------------

				pvrQuaternion_t quat_double;
				if (!PicoPlugin::f_PVR_GetTrackedPose(quat_double))
				{
					goto free;
				}

				videoItem->Texture->rt->orientation.w = quat_double.w;
				videoItem->Texture->rt->orientation.x = quat_double.x;
				videoItem->Texture->rt->orientation.y = quat_double.y;
				videoItem->Texture->rt->orientation.z = quat_double.z;

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

int FfmpegOnline::ReadThread(void* ptr)
{
	//!<start,  play the video.   $Nico@2016-11-2 09:42:59
	//!<
	//!<
	FfmpegOnline* instance = reinterpret_cast<FfmpegOnline*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;
	int32_t isEnd = 0;

	while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
	{
		//PRINT_LOG("ReadThread\n");

		if (Ffvs->m_iSeekFlag == SeekStatus::SEEK_TIME)
		{
			instance->SeekWithCache();
			continue;
		}

		if (CacheFinishedFlag::CACHE_FINISHED == Ffvs->m_iCacheFinishedFlag)   //<! 缓冲完成，线程挂起 !>
		{
			SDL_Delay(1);
			continue;
		}

		if (instance->m_ui64CurrentCacheSpaceInKB > instance->m_ui64CacheSpaceInKB)//<! 当缓冲进度大于设定最大缓存空间时，等待 !>
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
			if (AVERROR_EOF == isEnd)    //<! 读取到文件末尾时根据是否循环播放进行跳转 !>
			{
				Ffvs->m_iCacheFinishedFlag = CacheFinishedFlag::CACHE_FINISHED;    //<! 缓存结束标志 !>
				instance->SetLastCacheItem();
			}
			else if (AVERROR_HTTP_BAD_REQUEST == isEnd
				|| AVERROR_HTTP_UNAUTHORIZED == isEnd
				|| AVERROR_HTTP_FORBIDDEN == isEnd
				|| AVERROR_HTTP_NOT_FOUND == isEnd
				|| AVERROR_HTTP_OTHER_4XX == isEnd
				|| AVERROR_HTTP_SERVER_ERROR == isEnd)
			{
				Ffvs->m_iNetworkStatusFlag = NetworkStatus::NETWORK_CONNECTION_ERROR;
			}
			else if (AVERROR_INVALIDDATA == isEnd)
			{
				Ffvs->m_iNetworkStatusFlag = NetworkStatus::NETWORK_INVALID_DATA;
			}
		}
		else
		{
			instance->CreateCacheItemFromPacket(Ffvs->m_pflush_pkt);
		}
		av_packet_unref(Ffvs->m_pflush_pkt);

	}
	//!<end,  play the video.   $Nico@2016-11-2 09:42:59
	//!<
	//!<

	SDL_CondSignal(Ffvs->m_AudioQueue->audioCond);
	SDL_CondSignal(Ffvs->m_VideoQueue->videoCond);
	SDL_CondSignal(instance->m_CacheQueue->cacheCond);
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
	if (nullptr != instance->m_pCacheThread)
	{
		SDL_WaitThread(instance->m_pCacheThread, NULL);
	}

	//int64_t _iRetVal = instance->DeleteFold(instance->m_lSystemTempPath);

	instance->ReleaseSources();

	//!<
	PRINT_LOG("ReadThread Returned.\n");

	return RETURN_VALUE_SUCCESS;
}


int32_t FfmpegOnline::SetLastCacheItem()
{
	if (nullptr == m_CacheQueue)
	{
		return RETURN_VALUE_FAILED;
	}

	SDL_LockMutex(m_CacheQueue->cacheMutex);

	if (m_CacheQueue->LastItem != NULL)
	{
		m_CacheQueue->LastItem->isLastItem = 1;//<! 读到文件最后，设置最后一个item标志 !>
	}

	SDL_UnlockMutex(m_CacheQueue->cacheMutex);

	return RETURN_VALUE_SUCCESS;
}

int32_t FfmpegOnline::SeekWithCache()
{

	m_pVideoState->m_dCurrentAudioPts = m_pVideoState->m_iSeekTime;   //<! 将seek时间赋值给当前时间，给进度条显示 !>
	m_pVideoState->m_dCurrentVideoPts = m_pVideoState->m_iSeekTime;   //<! 将seek时间赋值给当前时间，给进度条显示 !>

	if (SeekInCacheFlag::SEEK_IN_CACHE == m_pVideoState->m_iSeekInCacheFlag)//<! 跳转到缓冲区中间并且不是向前跳转 !>
	{
		SeekInCacheArea(this->m_CacheQueue, m_pVideoState->m_iSeekTime);
	}
	else if (SeekInCacheFlag::SEEK_NOT_IN_CACHE == m_pVideoState->m_iSeekInCacheFlag)
	{

		m_pVideoState->m_dCurrentCacheAudioPts = m_pVideoState->m_iSeekTime;   //<! 将seek时间赋值给当前cache时间，给进度条显示 !>
		m_pVideoState->m_dCurrentCacheVideoPts = m_pVideoState->m_iSeekTime;   //<! 将seek时间赋值给当前cache时间，给进度条显示 !>


		double _seek_offset = 0;
		if (STREAM_TRACK_EMPTY == m_pVideoState->m_iAudioStream)  //<! 某些视频 例如m3u8，第一帧的时间不是0，因此需要偏移 !>
		{
			_seek_offset = m_pVideoState->m_sFirstFrame.FirstFramePts;
		}
		else
		{
			_seek_offset = m_pVideoState->m_sFirstFrame.FirstFramePts;
		}

		int32_t ret = av_seek_frame(
			m_pVideoState->m_pFormatCtx,
			-1,
			(m_pVideoState->m_iSeekTime + _seek_offset) * AV_TIME_BASE,
			AVSEEK_FLAG_BACKWARD
		);

		if (ret >= 0 && m_pVideoState->m_iBufferCleanFlag == BufferCleanFlag::BUFFER_CLEAN)  //<! 当前所有跳转都清除缓存。 !>
		{   //<! 清空缓存 !>
			this->ClearVideoQueue(m_pVideoState->m_VideoQueue);
			this->ClearAudioQueue(m_pVideoState->m_AudioQueue);
			this->ClearCacheQueue(this->m_CacheQueue);
			avio_flush(m_pVideoState->m_pFormatCtx->pb);
			avformat_flush(m_pVideoState->m_pFormatCtx);

			m_pVideoState->m_iCacheFinishedFlag = CacheFinishedFlag::CACHE_NOT_FINISHED;  //<! 重置缓存进度条 !>
			m_pVideoState->m_iIsSeekSuccessFlag = SeekSuccessFlag::SEEK_SUCCESS;//<! seek 成功 !>
			//m_pVideoState->m_iPacketThrowCount = PACKET_THROW_COUNT_AFTER_SEEK;
		}
		else
		{
			m_pVideoState->m_iIsSeekSuccessFlag = SeekSuccessFlag::SEEK_FAIL;//<! seek 不成功!>
			PRINT_LOG("Seek failed!\n");
		}
	}
	m_pVideoState->m_iSeekFlag = SeekStatus::SEEK_NOTHING;
	return RETURN_VALUE_SUCCESS;
}




void FfmpegOnline::SeekInCacheArea(CacheQueue* cq, double seekTime)
{
	if (nullptr == cq)
	{
		return;
	}

	CacheQueueItem *_First, *_Second;

	SDL_LockMutex(cq->cacheMutex);

	for (; cq->FirstItem != NULL;)
	{
		_First = cq->FirstItem;

		_Second = _First->Next;

		if ((_First->dts <= seekTime) && (_Second->dts >= seekTime))
			break;

		RemoveCacheFileStorageSizeFromThis(_First->storageSize);
		DeleteFile(_First->url);
		av_free(_First);
		cq->FirstItem = _Second;
		cq->Length--;
	}
	SDL_UnlockMutex(cq->cacheMutex);
}
int32_t FfmpegOnline::InitCacheQueue(CacheQueue** cq)
{
	if (*cq == nullptr)
	{
		*cq = (CacheQueue *)av_malloc(sizeof(CacheQueue));
	}
	memset(*cq, 0, sizeof(CacheQueue));

	(*cq)->cacheMutex = SDL_CreateMutex();
	CHECK_PTR_RETURN_IF_NULL((*cq)->cacheMutex, "cacheMutex is null! \n", RETURN_VALUE_FAILED);
	(*cq)->cacheCond = SDL_CreateCond();
	CHECK_PTR_RETURN_IF_NULL((*cq)->cacheCond, "cacheCond is null! \n", RETURN_VALUE_FAILED);

	return RETURN_VALUE_SUCCESS;
}

void FfmpegOnline::ClearCacheQueue(CacheQueue* cq)
{
	if (nullptr == cq)
	{
		return;
	}

	CacheQueueItem *item, *temp;

	SDL_LockMutex(cq->cacheMutex);

	for (item = cq->FirstItem; item != NULL; item = temp)
	{
		temp = item->Next;//
		DeleteFile(item->url);
		RemoveCacheFileStorageSizeFromThis(item->storageSize);
		av_free(item);
		cq->Length--;
	}
	cq->FirstItem = NULL;
	cq->LastItem = NULL;
	SDL_UnlockMutex(cq->cacheMutex);
}

void FfmpegOnline::ClearCacheQueueWithoutDeleteFile(CacheQueue* cq)
{
	if (nullptr == cq)
	{
		return;
	}

	CacheQueueItem *item, *temp;

	SDL_LockMutex(cq->cacheMutex);

	for (item = cq->FirstItem; item != NULL; item = temp)
	{
		temp = item->Next;//
		RemoveCacheFileStorageSizeFromThis(item->storageSize);
		av_free(item);
		cq->Length--;
	}
	cq->FirstItem = NULL;
	cq->LastItem = NULL;
	SDL_UnlockMutex(cq->cacheMutex);
}


void FfmpegOnline::ReleaseCacheQueue(CacheQueue** cq)
{
	if (*cq != nullptr)
	{

		SDL_DestroyMutex((*cq)->cacheMutex);
		SDL_DestroyCond((*cq)->cacheCond);

		av_free(*cq);
	}

}

int32_t FfmpegOnline::PutItemIntoCacheQueue(CacheQueue* cq, CacheQueueItem* item)
{
	uint32_t result = 0;

	SDL_LockMutex(cq->cacheMutex);//加锁

	if (cq->Length < CacheBufferMaxSize)
	{
		if (!cq->FirstItem)//第一个item为null 则队列是空的
		{
			cq->FirstItem = item;
			cq->LastItem = item;
			cq->Length = 1;
		}
		else
		{
			cq->LastItem->Next = item;//添加到队列后面
			cq->Length++;
			cq->LastItem = item;//此item变成队列尾部
		}

		m_pVideoState->m_ui64CurrentCacheLength = cq->Length;

		//if (cq->Length >= CacheBufferMinSize)
		//{
		SDL_CondSignal(cq->cacheCond);//唤醒其他线程  如果缓冲区里面有几个数据后再唤醒 较好
		//}
		result = 1;
	}
	else///缓冲区的大小 大于设定值 则让线程等待
	{
		//PRINT_LOG("Cache Put Thread wait\n");
		SDL_CondWait(cq->cacheCond, cq->cacheMutex);//解锁  等待被唤醒
		//SDL_CondWaitTimeout(cq->cacheCond, cq->cacheMutex, SDL_THREAD_WAITTIMEOUT);
	}
	SDL_UnlockMutex(cq->cacheMutex);//解锁

	return result;
}

int32_t FfmpegOnline::GetItemFromCacheQueue(CacheQueue* cq, CacheQueueItem** item)
{
	uint32_t result = 0;

	SDL_LockMutex(cq->cacheMutex);

	if (cq->Length > 0)//<! 当缓存文件太少，暂停 !>
	{
		if (cq->FirstItem)//有数据
		{
			*item = cq->FirstItem;
			if (!cq->FirstItem->Next)//只有一个
			{
				cq->FirstItem = NULL;
				cq->LastItem = NULL;
			}
			else
			{
				cq->FirstItem = cq->FirstItem->Next;
			}
			cq->Length--;
			(*item)->Next = NULL;
			result = 1;
		}

		m_pVideoState->m_ui64CurrentCacheLength = cq->Length;
		//if (cq->Length <= CacheBufferMinSize)
		//{
		SDL_CondSignal(cq->cacheCond);//唤醒下载线程
		//}
	}
	else
	{
		//PRINT_LOG("Cache Get Thread wait\n");
		SDL_CondWait(cq->cacheCond, cq->cacheMutex);//解锁  等待被唤醒
		//SDL_CondWaitTimeout(cq->cacheCond, cq->cacheMutex, SDL_THREAD_WAITTIMEOUT);
	}

	SDL_UnlockMutex(cq->cacheMutex);

	return result;
}

int FfmpegOnline::CacheReadThread(void* ptr)
{
	FfmpegOnline* instance = reinterpret_cast<FfmpegOnline*>(ptr);
	FfVideoState* Ffvs = instance->m_pVideoState;

	while (Ffvs->m_iPlayingFlag != PlayStatus::STOP)
	{

		if (Ffvs->m_iPlayingFlag == PlayStatus::PAUSE)
		{
			SDL_Delay(1);
			continue;
		}

		if (CacheFinishedFlag::CACHE_FINISHED != Ffvs->m_iCacheFinishedFlag)  //<! 当缓冲已经完成，不管是否满足最少条件直接播放!>
		{
			if (Ffvs->m_ui64CurrentCacheLength < MIN_CACHE_FILE_COUNT)
			{
				Ffvs->m_iNetworkBufferingStatusFlag = NetworkBufferingStatus::NETWORK_BUFFER_BUFFERING;
				SDL_Delay(1);
				continue;
			}
			else
			{
				if ((NetworkBufferingStatus::NETWORK_BUFFER_BUFFERING == Ffvs->m_iNetworkBufferingStatusFlag) && Ffvs->m_ui64CurrentCacheLength < MIN_BUFFER_STATUS_FRAME_COUNT)
				{//<! 缓冲过，但是没有播放，继续进行缓冲 !>
					SDL_Delay(1);
					continue;
				}
				Ffvs->m_iNetworkBufferingStatusFlag = NetworkBufferingStatus::NETWORK_BUFFER_NORMAL;
			}
		}
		else
		{
			Ffvs->m_iNetworkBufferingStatusFlag = NetworkBufferingStatus::NETWORK_BUFFER_NORMAL;
		}

		CacheQueueItem* cacheItem = nullptr;
		//从队列中拿出音频数据
		if (instance->GetItemFromCacheQueue(instance->m_CacheQueue, &cacheItem))
		{

			AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
			av_init_packet(packet);
			av_new_packet(packet, cacheItem->size);

			instance->ReadPacketFromCacheFile(packet, cacheItem->url);

			instance->RemoveCacheFileStorageSizeFromThis(cacheItem->storageSize);

			//double _time;
			if (packet->stream_index == Ffvs->m_iVideoStream)
			{
				instance->CreateVideoItemFromPacket(packet);
			}
			else if (packet->stream_index == Ffvs->m_iAudioStream)
			{
				instance->CreateAudioItemFromPacket(packet);
			}

			Ffvs->m_iPlayFinishedFlag = (cacheItem->isLastItem == 1 ? PlayFinishedFlag::PLAY_FINISHED : PlayFinishedFlag::PLAY_NOT_FINISHED);
			//<! 判断是否为最后一帧，置位 !>

			av_packet_unref(packet);
			av_free(packet);
		}
	free:
		av_free(cacheItem);
	}


	PRINT_LOG("CacheThread Returned.\n");
	return RETURN_VALUE_SUCCESS;

}


int32_t FfmpegOnline::CreateCacheItemFromPacket(AVPacket* packet)
{
	TCHAR _lpFilename[MAX_PATH] = { 0 };
	CreateUsableCacheFileName(_lpFilename);
	int64_t _size = WriteCacheFileFromPacket(packet, _lpFilename);
	if (0 > _size)
	{
		PRINT_LOG("CreatePacketCacheFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}


	CacheQueueItem* _cacheItem = nullptr;
	_cacheItem = (CacheQueueItem*)av_mallocz(sizeof(CacheQueueItem));

	CHECK_PTR_RETURN_IF_NULL(_cacheItem, "Cache item malloc falied!\n", RETURN_VALUE_FAILED);

	wcscpy(_cacheItem->url, _lpFilename);
	_cacheItem->dts = CalcCacheItemDts(packet);
	_cacheItem->size = packet->size;
	_cacheItem->storageSize = CalculateStorageSpaceInKB(_size);//<! 占用磁盘空间大小,最少4KB !>
	_cacheItem->streamIndex = packet->stream_index;
	_cacheItem->Next = NULL;

	FirstFrameCheck(&(m_pVideoState->m_sFirstFrame), _cacheItem->dts);

	while (!PutItemIntoCacheQueue(this->m_CacheQueue, _cacheItem) && (m_pVideoState->m_iPlayingFlag != PlayStatus::STOP));

	AddCachedPtsToThis(_cacheItem);
	AddCacheFileStorageSizeToThis(_cacheItem->storageSize); //<! 记录创建文件总大小 !>

	return RETURN_VALUE_SUCCESS;
}


double FfmpegOnline::CalcCacheItemDts(AVPacket* packet)
{

	double _ret = 0;
	if (packet->stream_index == m_pVideoState->m_iVideoStream)
	{
		_ret = (packet->dts) * av_q2d(m_pVideoState->m_pFormatCtx->streams[m_pVideoState->m_iVideoStream]->time_base);
	}
	else if (packet->stream_index == m_pVideoState->m_iAudioStream)
	{
		_ret = (packet->dts) * av_q2d(m_pVideoState->m_pFormatCtx->streams[m_pVideoState->m_iAudioStream]->time_base);
	}
	return _ret;
}

int32_t FfmpegOnline::AddCachedPtsToThis(CacheQueueItem* cache_item)
{
	if (cache_item->streamIndex == m_pVideoState->m_iVideoStream)
	{
		m_pVideoState->m_dCurrentCacheVideoPts = (cache_item->dts);
	}
	else if (cache_item->streamIndex == m_pVideoState->m_iAudioStream)
	{
		m_pVideoState->m_dCurrentCacheAudioPts = (cache_item->dts);
	}

	return RETURN_VALUE_SUCCESS;

}

int32_t FfmpegOnline::AddCacheFileStorageSizeToThis(int64_t size)
{
	this->m_ui64CurrentCacheSpaceInKB += size;
	return RETURN_VALUE_SUCCESS;
}

int32_t FfmpegOnline::RemoveCacheFileStorageSizeFromThis(int64_t size)
{
	this->m_ui64CurrentCacheSpaceInKB -= size;
	return RETURN_VALUE_SUCCESS;
}

uint64_t FfmpegOnline::CalculateStorageSpaceInKB(uint64_t sizeInBytes)//<! 单个文件最小占用空间为4K，因为其中有一些簇需要存储 !>
{
	if (sizeInBytes < MIN_FILE_SIZE_IN_BYTES)
	{
		sizeInBytes = MIN_FILE_SIZE_IN_BYTES;
	}

	uint64_t _toKB = ceil(sizeInBytes >> 10);

	return _toKB;
}


int32_t FfmpegOnline::InitThread()
{
	this->m_pReadThread = SDL_CreateThread(&FfmpegOnline::ReadThread, "ReadThread", this);  //<! 读取线程
	CHECK_PTR_RETURN_IF_NULL(this->m_pReadThread, "ReadThread null ptr! \n", RETURN_VALUE_FAILED);

	if (m_pVideoState->m_iAudioStream != STREAM_TRACK_EMPTY)//<! 当有音频时创建，否则不创建 !>
	{
		this->m_pAudioThread = SDL_CreateThread(&FfmpegOnline::AudioThread, "AudioThread", this); //!< 音频播放线程
		CHECK_PTR_RETURN_IF_NULL(this->m_pAudioThread, "AudioThread null ptr! \n", RETURN_VALUE_FAILED);
	}

	if (m_pVideoState->m_iVideoStream != STREAM_TRACK_EMPTY)//<! 当有视频时创建，否则不创建 !>
	{
		this->m_pVideoThread = SDL_CreateThread(&FfmpegOnline::VideoThread, "VideoThread", this); //<! 视频播放线程
		CHECK_PTR_RETURN_IF_NULL(this->m_pVideoThread, "FfmpegOnlineVideoThread null ptr! \n", RETURN_VALUE_FAILED);
		//this->m_pExtClockThread = SDL_CreateThread(&FfmpegOnline::ExtClockThread, "ExtClockThread", this); //<! 外部时钟线程作为视频同步源
	}


#if USE_SDL_VIDEO
	this->m_pSDLVideoThread = SDL_CreateThread(&FfmpegOnline::SDLVideoThread, "SDLVideoThread", this); //<! 视频播放线程
	CHECK_PTR_RETURN_IF_NULL(this->m_pVideoThread, "SDLVideoThread null ptr! \n", RETURN_VALUE_FAILED);
#endif //

	this->m_pCacheThread = SDL_CreateThread(&FfmpegOnline::CacheReadThread, "CacheReadThread", this); //<! 视频缓冲线程
	CHECK_PTR_RETURN_IF_NULL(this->m_pCacheThread, "FfmpegOnlineCacheReadThread null ptr! \n", RETURN_VALUE_FAILED);



	PRINT_LOG("Init Thread ok!\n");
	return RETURN_VALUE_SUCCESS;
}

#define MAX_CURRENT_TIME_BUFFERSIZE 19

uint32_t FfmpegOnline::CreateUsableCacheFileName(TCHAR* entirePath)
{
	int64_t _iTime = av_gettime();
	TCHAR _lpTime[MAX_CURRENT_TIME_BUFFERSIZE];
	_i64tow_s(_iTime, _lpTime, MAX_CURRENT_TIME_BUFFERSIZE, 16);
	//swprintf(_lpTime,L"%S",_iTime);
	//int _iLength = sizeof(_lpTime);
	TCHAR _lpTmpFoldPath[MAX_PATH] = { 0 };
	wcscpy(_lpTmpFoldPath, this->m_lSystemTempPath);
	wcscat_s(_lpTmpFoldPath, L"\\");
	wcscat_s(_lpTmpFoldPath, _lpTime);//<! 使用当前时间的16进制数当做文件名，microseconds 为单位 !>
	wcscpy(entirePath, _lpTmpFoldPath);
	return RETURN_VALUE_SUCCESS;
}

int64_t FfmpegOnline::WriteCacheFileFromPacket(AVPacket* packet, LPCTSTR cacheFoldName)  //<! 返回值为文件的占用空间
{
	TCHAR _lpFileName[MAX_PATH];
	wcscpy(_lpFileName, cacheFoldName);

	HANDLE _handle = CreateFile(_lpFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == _handle)
	{
		DWORD _dLastError = GetLastError();
		PRINT_LOG("WriteFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}

	DWORD _dwFileWritten = 0;
	BOOL _bRet = WriteFile(_handle, packet, sizeof(AVPacket), &_dwFileWritten, NULL);
	if (NULL == _bRet)
	{
		PRINT_LOG("WriteFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}

	_bRet = WriteFile(_handle, packet->data, packet->size, &_dwFileWritten, NULL);
	if (NULL == _bRet)
	{
		PRINT_LOG("WriteFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}

	//if (packet->buf) { //<! 不用复制，因为buf->data和packet->data指向同一个内存单元 !>

	//_bRet = WriteFile(_handle, packet->buf->data, packet->buf->size, &_dwFileWritten, NULL);
	//if (NULL == _bRet)
	//{
	//	PRINT_LOG("WriteFile Failed!\n");
	//	return RETURN_VALUE_FAILED;
	//}
	//}
	if (packet->side_data) {

		_bRet = WriteFile(_handle, packet->side_data, sizeof(AVPacketSideData), &_dwFileWritten, NULL);
		if (NULL == _bRet)
		{
			PRINT_LOG("WriteFile Failed!\n");
			return RETURN_VALUE_FAILED;
		}

		_bRet = WriteFile(_handle, packet->side_data->data, packet->side_data->size, &_dwFileWritten, NULL);
		if (NULL == _bRet)
		{
			PRINT_LOG("WriteFile Failed!\n");
			return RETURN_VALUE_FAILED;
		}
	}

	FlushFileBuffers(_handle);

	FILE_STANDARD_INFO fsi = { 0 };
	GetFileInformationByHandleEx(_handle, FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO));


	CloseHandle(_handle);

	return (fsi.AllocationSize.QuadPart);
}


int64_t FfmpegOnline::ReadPacketFromCacheFile(AVPacket* packet, LPCTSTR cacheFoldName)  //<! 返回值为文件的占用空间
{

	TCHAR _lpFileName[MAX_PATH];
	wcscpy(_lpFileName, cacheFoldName);
	HANDLE _handle = CreateFile(_lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
	//<! 打开文件，设置属性为关闭后删除，因此当文件关闭后，由Windows决定什么时候删除。 !>
	if (INVALID_HANDLE_VALUE == _handle)
	{
		DWORD _dLastError = GetLastError();
		PRINT_LOG("CreateFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}


	AVBufferRef* _pPacketAVBufferRef = packet->buf;
	uint8_t* _pPacketData = packet->data;
	DWORD _dwFileWritten = 0;
	BOOL _bRet = ReadFile(_handle, packet, sizeof(AVPacket), &_dwFileWritten, NULL);
	if (NULL == _bRet)
	{
		PRINT_LOG("ReadFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}


	packet->buf = _pPacketAVBufferRef;
	packet->data = _pPacketData;
	_bRet = ReadFile(_handle, packet->data, packet->size, &_dwFileWritten, NULL);
	if (NULL == _bRet)
	{
		PRINT_LOG("ReadFile Failed!\n");
		return RETURN_VALUE_FAILED;
	}

	if (packet->side_data)
	{
		packet->side_data = (AVPacketSideData *)av_mallocz(sizeof(AVPacketSideData));
		ReadFile(_handle, packet->side_data, sizeof(AVPacketSideData), &_dwFileWritten, NULL);
		packet->side_data->data = (uint8_t *)av_mallocz(packet->side_data->size);
		ReadFile(_handle, packet->side_data->data, packet->side_data->size, &_dwFileWritten, NULL);
		if (NULL == _bRet)
		{
			PRINT_LOG("ReadFile side_data Failed!\n");
			return RETURN_VALUE_FAILED;
		}
	}


	FILE_STANDARD_INFO fsi = { 0 };
	GetFileInformationByHandleEx(_handle, FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO));

	_bRet = CloseHandle(_handle);
	if (NULL == _bRet)
	{
		PRINT_LOG("CloseHandle Failed!\n");
		return RETURN_VALUE_FAILED;
	}

	return (fsi.AllocationSize.QuadPart);
}


uint64_t FfmpegOnline::GetFreeSpaceOfPath(TCHAR* in_file_path, uint32_t path_length)  //<! 返回in_file_path目录剩余可被使用的空间 !>
{
	uint64_t _i64FreeBytesAvailable = 0;
	uint64_t _i64TotalNumberOfBytes = 0;
	uint64_t _i64TotalNumberOfFreeBytes = 0;

	TCHAR* _temp = (TCHAR*)malloc(path_length);
	wcscpy(_temp, in_file_path);
	GetDiskFreeSpaceEx(_temp, (PULARGE_INTEGER)&_i64FreeBytesAvailable, (PULARGE_INTEGER)&_i64TotalNumberOfBytes, (PULARGE_INTEGER)&_i64TotalNumberOfFreeBytes);
	free(_temp);

	//<! Bytes to MB!>
	uint64_t _inKB = (_i64FreeBytesAvailable >> 10);

	return  _inKB;
}

int32_t FfmpegOnline::CreateTempFoldFromSystemTempPath(TCHAR* out_tempPath, uint32_t length)
{
	if (NULL == GetTempPath(length, out_tempPath))
	{
		PRINT_LOG("System temp file not found! \n");
		return RETURN_VALUE_FAILED;
	}

	uint64_t _freeKBCanBeUsed = GetFreeSpaceOfPath(out_tempPath, length); //<! 获取目录剩余空间，以此确定缓存大小 , 取剩余空间的5%，最多100MB !>

	if (_freeKBCanBeUsed >= MAX_CACHE_SPACE_IN_KB)
	{
		this->m_ui64CacheSpaceInKB = MAX_CACHE_SPACE_IN_KB;
	}
	else
	{
		this->m_ui64CacheSpaceInKB = _freeKBCanBeUsed * 0.2;
	}


	if (0 != wcscat_s(out_tempPath, length, L"PicoPlayer"))
	{
		PRINT_LOG("Could not create PicoPlayer tmp fold!");
		return RETURN_VALUE_FAILED;
	}

	bool _retVal = DeleteFold(out_tempPath);

	if (0 == CreateDirectory(out_tempPath, NULL))
	{
		PRINT_LOG("Could not create PicoPlayer tmp fold!");
		return RETURN_VALUE_FAILED;
	}

	return RETURN_VALUE_SUCCESS;
}

bool FfmpegOnline::DeleteFold(const TCHAR* lpszPath)   //<! 若已存在，则删除成功返回true。 不存在删除失败返回false !>
{
	SHFILEOPSTRUCT _fileOp = { 0 };
	_fileOp.fFlags = FOF_NOCONFIRMATION |    //<!不出现确认对话框
		FOF_SILENT; //<!不出现进度条


	TCHAR _tmpPath[MAX_PATH];
	wcscpy(_tmpPath, lpszPath);

	wcscat(_tmpPath, TEXT("\\"));//<!后面必须要有一个 \0 ，否则执行不成功   
	TCHAR* t = wcsrchr(_tmpPath, L'\\');
	*(t) = 0;

	_fileOp.pFrom = _tmpPath;
	_fileOp.pTo = NULL;      //<!一定要是NULL
	_fileOp.wFunc = FO_DELETE;    //<!删除操作
	return SHFileOperation(&_fileOp) == 0;
}

int32_t FfmpegOnline::InitDecoder()
{

	if (Ffmpeg::InitDecoder() != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Init Decoder failed\n");
		return RETURN_VALUE_FAILED;
	}

	if (InitCacheQueue(&m_CacheQueue) != RETURN_VALUE_SUCCESS)
	{
		PRINT_LOG("Could not init cache queue!\n");
		return RETURN_VALUE_FAILED;
	}
	return RETURN_VALUE_SUCCESS;
}

int32_t FfmpegOnline::ReleaseDecoder()
{
	Ffmpeg::ReleaseDecoder();

	if (nullptr != this->m_CacheQueue)
	{
		ClearCacheQueueWithoutDeleteFile(this->m_CacheQueue);  //<! 加快退出速度，不清空缓存 !>
		ReleaseCacheQueue(&this->m_CacheQueue);
	}

	return RETURN_VALUE_SUCCESS;
}

