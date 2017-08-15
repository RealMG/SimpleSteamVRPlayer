#pragma once

#ifdef FFNATIVEMOVIE_EXPORTS
#define FFNATIVEMOVIE_API __declspec(dllexport) 
#else
#define FFNATIVEMOVIE_API __declspec(dllimport)
#endif

#include <string>
#include "Decoders/ffmpeg_local.h"
#include "Decoders/ffmpeg_online.h"


extern "C" {


	FFNATIVEMOVIE_API int32_t Create(MovieType movieType, HWAccel hwAccel);

	FFNATIVEMOVIE_API void Destroy();

	FFNATIVEMOVIE_API int32_t OpenMovie( LPCWSTR filename, Video3DMode mode, int32_t flag);

	FFNATIVEMOVIE_API int32_t GetMovieWidth();

	FFNATIVEMOVIE_API int32_t GetMovieHeight();

	FFNATIVEMOVIE_API uint64_t GetMovieDuration();

	FFNATIVEMOVIE_API int32_t GetIsPlayFinished();

	FFNATIVEMOVIE_API int32_t Play();

	FFNATIVEMOVIE_API int32_t Pause();

	FFNATIVEMOVIE_API int32_t Stop();

	FFNATIVEMOVIE_API int32_t SeekTo( LONGLONG position);

	FFNATIVEMOVIE_API int32_t GetPosition(uint64_t* position);

	FFNATIVEMOVIE_API int32_t VolumeDown( int32_t value);

	FFNATIVEMOVIE_API int32_t VolumeUp( int32_t value);

	FFNATIVEMOVIE_API int32_t SetVolume(int32_t value);

	FFNATIVEMOVIE_API int32_t GetCachedPosition(uint64_t* position);

	FFNATIVEMOVIE_API int32_t GetIsBuffering();

	FFNATIVEMOVIE_API int32_t SetWindowHandle(HWND handle);

	FFNATIVEMOVIE_API int32_t SetWindowSize(int32_t width, int32_t height);

}