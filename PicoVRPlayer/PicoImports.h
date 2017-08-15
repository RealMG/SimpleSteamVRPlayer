#pragma once


enum MovieType
{
	ONLINE,
	LOCAL
};

enum Video3DMode
{
	Off,
	LR,
	UD
};


typedef int(*Create)(MovieType movieType, int n);

typedef void(*Destroy)();

typedef int(*OpenMovie)(wchar_t* filename, Video3DMode mode, int flag);

typedef int(*GetMovieWidth)();

typedef int(*GetMovieHeight)();

typedef unsigned long long(*GetMovieDuration)();

typedef int(*GetIsPlayFinished)();

typedef int(*Play)();

typedef int(*Pause)();

typedef int(*Stop)();

typedef int(*SeekTo)(long long position);

typedef int(*GetPosition)(unsigned int* position);

typedef int(*VolumeDown)(int value);

typedef int(*VolumeUp)(int value);

typedef int(*SetVolume)(int value);

typedef int(*GetCachedPosition)(unsigned int* position);

typedef int(*GetIsBuffering)();

typedef int(*SetWindowHandle)(HWND handle);

typedef int(*SetWindowSize)(int width, int height);


class FPicoPlayerImports
{
public:
	static Create			  f_Create;
	static Destroy           f_Destroy;
	static OpenMovie         f_OpenMovie;
	static GetMovieWidth     f_GetMovieWidth;
	static GetMovieHeight    f_GetMovieHeight;
	static GetMovieDuration  f_GetMovieDuration;
	static GetIsPlayFinished f_GetIsPlayFinished;
	static Play              f_Play;
	static Pause             f_Pause;
	static Stop              f_Stop;
	static SeekTo            f_SeekTo;
	static GetPosition       f_GetPosition;
	static VolumeDown        f_VolumeDown;
	static VolumeUp          f_VolumeUp;
	static SetVolume         f_SetVolume;
	static GetCachedPosition f_GetCachedPosition;
	static GetIsBuffering    f_GetIsBuffering;
	static SetWindowHandle   f_SetWindowHandle;
	static SetWindowSize     f_SetWindowSize;

	static bool Initialize();
	static void Uninitialize();

private:
	static HMODULE DllModule;
	FPicoPlayerImports();
	~FPicoPlayerImports();
};

