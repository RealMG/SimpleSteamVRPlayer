#include "FFnativeMovie.h"
#include <memory>

SESSION* GSession = nullptr;

FFNATIVEMOVIE_API int32_t Create(MovieType movieType, HWAccel hwAccel)
{
	PRINT_LOG("CreateSession\n");

	if (nullptr == GSession)
	{
		GSession = (SESSION*)calloc(sizeof(SESSION), 1);
		switch (movieType)
		{
		case MovieType::ONLINE:
			GSession->player = new FfmpegOnline();
			break;
		case MovieType::LOCAL:
			GSession->player = new FfmpegLocal();
			break;
		}
		if (GSession->player == nullptr)
			goto Error;
	}

	//int ret = OpenMovie(L"D:\\WorkspaceForNico\\TestingVideos\\NBA.mp4",Video3DMode_Off,-1);
	//Play();

	return RETURN_VALUE_SUCCESS;

Error:
	free(GSession);
	GSession = nullptr;
	return RETURN_VALUE_FAILED;
}


FFNATIVEMOVIE_API void Destroy()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "DestroySession GSession is null!\n", );
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "DestroySession GSession->player is null!\n",);

	if (GSession->player)
	{
		delete(GSession->player);
		GSession->player = nullptr;
	}

	free(GSession);
	GSession == nullptr;
	//_CrtDumpMemoryLeaks();
}


FFNATIVEMOVIE_API int32_t GetIsPlayFinished()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetIsPlayFinished GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetIsPlayFinished();
}

//FFNATIVEMOVIE_API int32_t SetTexturePointers(ID3D11Resource * leftEyeTexture, ID3D11Resource * rightEyeTexture)
//{
//	CHECK_PTR_RETURN_IF_NULL(GSession, "SetTexturePointers GSession is null!\n", RETURN_VALUE_FAILED);
//	CHECK_PTR_RETURN_IF_NULL(leftEyeTexture, "leftEyeTexture is null!\n", RETURN_VALUE_FAILED);
//
//	//GSession->leftEyeTexture = leftEyeTexture;
//	//GSession->rightEyeTexture = rightEyeTexture;
//
//	if (GSession->player->InitD3d11(leftEyeTexture) > 0)
//	{
//		GSession->player->Play();
//	}
//
//	return 0;
//}

FFNATIVEMOVIE_API int32_t OpenMovie(LPCWSTR filename, Video3DMode mode, int32_t flag)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "OpenMovie GSession is null!\n", RETURN_VALUE_FAILED);

	GSession->mode = mode;
	GSession->lastFrameUploaded = -1;


	DWORD num = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, 0);

	if (num > 0)
	{
		char* buf = (char*)malloc(num * sizeof(char));
		WideCharToMultiByte(CP_ACP, 0, filename, -1, buf, num, NULL, 0);
		int32_t ret = GSession->player->Open(buf);
		free(buf);
		return ret;
	}

	return RETURN_VALUE_FAILED;

}

//FFNATIVEMOVIE_API int32_t GetLastFrameUploaded()
//{
//	CHECK_PTR_RETURN_IF_NULL(GSession, "GetLastFrameUploaded GSession is null!\n", RETURN_VALUE_FAILED);
//
//	return GSession->player->GetLastFrameCount();
//}

FFNATIVEMOVIE_API int32_t GetMovieWidth()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetMovieWidth GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetMovieWidth();
}

FFNATIVEMOVIE_API int32_t GetMovieHeight()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetMovieHeight GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetMovieHeight();
}

FFNATIVEMOVIE_API uint64_t GetMovieDuration()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetMovieDuration GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetFilmDuration();
}

FFNATIVEMOVIE_API int32_t Play()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "Play GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->Play();
}

FFNATIVEMOVIE_API int32_t Pause()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "Pause GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->Pause();
}

FFNATIVEMOVIE_API int32_t Stop()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "Stop GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->Stop();
}

FFNATIVEMOVIE_API int32_t SeekTo(LONGLONG position)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "SeekTo GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->SeekTo(position, BufferCleanFlag::BUFFER_CLEAN);
}

FFNATIVEMOVIE_API int32_t GetPosition(uint64_t * position)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetPosition GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetPosition(position);
}

FFNATIVEMOVIE_API int32_t GetCachedPosition(uint64_t * position)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetCachedPosition GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetCachedPosition(position);
}


//FFNATIVEMOVIE_API int32_t ShowMsg(int32_t msg)
//{
//	return 0;
//}

//FFNATIVEMOVIE_API int32_t GerFrame(ID3D11Resource * leftEyeTexture, ID3D11Resource * rightEyeTexture)
//{
//	CHECK_PTR_RETURN_IF_NULL(GSession, "GerFrame GSession is null!\n", RETURN_VALUE_FAILED);
//
//	GSession->player->GetFrame(GSession, leftEyeTexture);
//	return 0;
//}

//FFNATIVEMOVIE_API int32_t UpdateCanCopy()
//{
//	return 0;
//}

FFNATIVEMOVIE_API int32_t VolumeDown(int32_t value)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "VolumeDown GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->VolumeDown(value);
}

FFNATIVEMOVIE_API int32_t VolumeUp(int32_t value)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "VolumeUp GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->VolumeUp(value);
}

FFNATIVEMOVIE_API int32_t SetVolume(int32_t value)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetEnd GSession is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->SetVolume(value);
}

//FFNATIVEMOVIE_API int32_t SetMovieReplay(ReplayType replayType)
//{
//	CHECK_PTR_RETURN_IF_NULL(GSession, "SetMovieReplay GSession is null!\n", RETURN_VALUE_FAILED);
//	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);
//
//	return GSession->player->SetMovieReplay(replayType);
//}

FFNATIVEMOVIE_API int32_t GetIsBuffering()
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "GetIsBuffering GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	return GSession->player->GetIsBuffering();
}

FFNATIVEMOVIE_API int32_t SetWindowHandle(HWND handle)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "SetWindowHandle GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	GSession->player->SetHWND(handle);

	return RETURN_VALUE_SUCCESS;
}

FFNATIVEMOVIE_API int32_t SetWindowSize(int32_t width, int32_t height)
{
	CHECK_PTR_RETURN_IF_NULL(GSession, "SetWindowSize GSession is null!\n", RETURN_VALUE_FAILED);
	CHECK_PTR_RETURN_IF_NULL(GSession->player, "GetIsPlayFinished GSession->player is null!\n", RETURN_VALUE_FAILED);

	if (width > 0 && height > 0)
	{
		GSession->player->SetWindowSize(width,height);
		return RETURN_VALUE_SUCCESS;
	}
	return RETURN_VALUE_FAILED;
}

