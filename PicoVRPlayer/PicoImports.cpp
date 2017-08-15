#include "stdafx.h"
#include "PicoImports.h"



#define DLL_IMPORT(FuncName) \
	f_##FuncName = (FuncName)GetProcAddress(DllModule, #FuncName); \
	if (f_##FuncName == nullptr) \
	{ \
		Uninitialize();\
		return false; \
	}


Create FPicoPlayerImports::f_Create = nullptr;
Destroy FPicoPlayerImports::f_Destroy = nullptr;
OpenMovie FPicoPlayerImports::f_OpenMovie = nullptr;
GetMovieWidth FPicoPlayerImports::f_GetMovieWidth = nullptr;
GetMovieHeight FPicoPlayerImports::f_GetMovieHeight = nullptr;
GetMovieDuration FPicoPlayerImports::f_GetMovieDuration = nullptr;
GetIsPlayFinished FPicoPlayerImports::f_GetIsPlayFinished = nullptr;
Play FPicoPlayerImports::f_Play = nullptr;
Pause FPicoPlayerImports::f_Pause = nullptr;
Stop FPicoPlayerImports::f_Stop = nullptr;
SeekTo FPicoPlayerImports::f_SeekTo = nullptr;
GetPosition FPicoPlayerImports::f_GetPosition = nullptr;
VolumeDown FPicoPlayerImports::f_VolumeDown = nullptr;
VolumeUp FPicoPlayerImports::f_VolumeUp = nullptr;

SetVolume FPicoPlayerImports::f_SetVolume = nullptr;

GetCachedPosition FPicoPlayerImports::f_GetCachedPosition = nullptr;
GetIsBuffering FPicoPlayerImports::f_GetIsBuffering = nullptr;
SetWindowHandle FPicoPlayerImports::f_SetWindowHandle = nullptr;
SetWindowSize FPicoPlayerImports::f_SetWindowSize = nullptr;

HMODULE FPicoPlayerImports::DllModule = nullptr;

bool FPicoPlayerImports::Initialize()
{
	DllModule = LoadLibraryA("PicoDecoder.dll");
	if (DllModule == nullptr)
	{
		DWORD aa = GetLastError();
		return false;
	}

	DLL_IMPORT(Create);
	DLL_IMPORT(Destroy);
	DLL_IMPORT(OpenMovie);
	DLL_IMPORT(GetMovieWidth);
	DLL_IMPORT(GetMovieHeight);
	DLL_IMPORT(GetMovieDuration);
	DLL_IMPORT(GetIsPlayFinished);
	DLL_IMPORT(Play);
	DLL_IMPORT(Pause);
	DLL_IMPORT(Stop);
	DLL_IMPORT(SeekTo);
	DLL_IMPORT(GetPosition);
	DLL_IMPORT(VolumeDown);
	DLL_IMPORT(VolumeUp);
	DLL_IMPORT(SetVolume);
	DLL_IMPORT(GetCachedPosition);
	DLL_IMPORT(GetIsBuffering);
	DLL_IMPORT(SetWindowHandle);
	DLL_IMPORT(SetWindowSize);

	return true;
}

void FPicoPlayerImports::Uninitialize()
{
	//::FreeLibrary(DllModule);
}

FPicoPlayerImports::FPicoPlayerImports()
{

}

FPicoPlayerImports::~FPicoPlayerImports()
{

}
