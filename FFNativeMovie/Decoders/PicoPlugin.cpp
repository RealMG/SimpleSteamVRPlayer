#include "../stdafx.h"
#include "PicoPlugin.h"

#define DLL_IMPORT(FuncName) \
	f_##FuncName = (FuncName)GetProcAddress(DllModule, #FuncName); \
	if (f_##FuncName == nullptr) \
	{ \
		Uninitialize(DllModule);\
		return false; \
	}


HMODULE PicoPlugin::DllModule = nullptr;
bool PicoPlugin::m_bIsInitialized = false;
PVR_Init PicoPlugin::f_PVR_Init = nullptr;

UE4InitWithRenderTargetSize PicoPlugin::f_UE4InitWithRenderTargetSize = nullptr;

PVR_GetTrackedPose PicoPlugin::f_PVR_GetTrackedPose = nullptr;
PVR_GetRenderTargetSize PicoPlugin::f_PVR_GetRenderTargetSize = nullptr;
SetTextureFromUnityATW PicoPlugin::f_SetTextureFromUnityATW = nullptr;
PVR_IsHmdConnected PicoPlugin::f_PVR_IsHmdConnected = nullptr;
UE4Shutdown PicoPlugin::f_UE4Shutdown = nullptr;
PVR_Shutdown PicoPlugin::f_PVR_Shutdown = nullptr;
UE4Initialization PicoPlugin::f_UE4Initialization = nullptr;
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
	DLL_IMPORT(UE4InitWithRenderTargetSize);
	DLL_IMPORT(PVR_GetTrackedPose);
	DLL_IMPORT(PVR_GetRenderTargetSize);
	DLL_IMPORT(SetTextureFromUnityATW);
	DLL_IMPORT(PVR_IsHmdConnected);
	DLL_IMPORT(UE4Shutdown);
	DLL_IMPORT(PVR_Shutdown);
	
	m_bIsInitialized = true;

	return m_bIsInitialized;
}

bool PicoPlugin::GetIsInitialized()
{
	return m_bIsInitialized;
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
