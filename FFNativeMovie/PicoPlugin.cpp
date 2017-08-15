#include "stdafx.h"
#include "PicoPlugin.h"

#define DLL_IMPORT(FuncName) \
	f_##FuncName = (FuncName)GetProcAddress(DllModule, #FuncName); \
	if (f_##FuncName == nullptr) \
	{ \
		Uninitialize(DllModule);\
		return false; \
	}



PVR_Init PicoPlugin::f_PVR_Init = nullptr;
UE4Initialization PicoPlugin::f_UE4Initialization = nullptr;
PVR_GetTrackedPose PicoPlugin::f_PVR_GetTrackedPose = nullptr;
PVR_GetRenderTargetSize PicoPlugin::f_PVR_GetRenderTargetSize = nullptr;
SetTextureFromUnityATW PicoPlugin::f_SetTextureFromUnityATW = nullptr;
PVR_IsHmdConnected PicoPlugin::f_PVR_IsHmdConnected = nullptr;
UE4Shutdown PicoPlugin::f_UE4Shutdown = nullptr;
PVR_Shutdown PicoPlugin::f_PVR_Shutdown = nullptr;

bool PicoPlugin::Initialize()
{
	DllModule = LoadLibraryA("PicoPlugin.dll");
	if (DllModule == nullptr)
	{
		DWORD aa = GetLastError();
		return false;
	}

	DLL_IMPORT(PVR_Init);
	DLL_IMPORT(UE4Initialization);
	DLL_IMPORT(PVR_GetTrackedPose);
	DLL_IMPORT(PVR_GetRenderTargetSize);
	DLL_IMPORT(SetTextureFromUnityATW);
	DLL_IMPORT(PVR_IsHmdConnected);
	DLL_IMPORT(UE4Shutdown);
	DLL_IMPORT(PVR_Shutdown);
}

void PicoPlugin::Uninitialize(void* DllHandle)
{
	::FreeLibrary((HMODULE)DllHandle);
}

PicoPlugin::PicoPlugin()
{
}


PicoPlugin::~PicoPlugin()
{
}
