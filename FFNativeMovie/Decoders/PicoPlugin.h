#pragma once

struct pvrQuaternion_t
{
	double w, x, y, z;
};


typedef struct pvrQuaternion_t_float_
{
	float x, y, z, w;
}quatf;

typedef struct _RenderTexture
{
	quatf orientation;
	void *LRT;
	void *RRT;

	_RenderTexture()
	{
		orientation = quatf();
		LRT = nullptr;
		RRT = nullptr;
	}

	_RenderTexture(quatf inQ, void *inL, void *inR)
	{
		orientation = inQ;
		LRT = inL;
		RRT = inR;
	}

} RenderTexture;

typedef bool(*PVR_Init)();


typedef void(*UE4InitWithRenderTargetSize)(void * D3D11device, int RenderTargetWidth, int RenderTargetHeight);


typedef bool(*PVR_GetTrackedPose)(pvrQuaternion_t& pose);

typedef bool(*PVR_GetRenderTargetSize)(int &width, int &height);

typedef void(*SetTextureFromUnityATW)(RenderTexture rt);

typedef bool(*PVR_IsHmdConnected)();

typedef bool(*UE4Shutdown)();

typedef bool(*PVR_Shutdown)();

typedef void(*UE4Initialization)(void * D3D11device);


class PicoPlugin
{

public:
	static PVR_Init					f_PVR_Init;
	static UE4InitWithRenderTargetSize f_UE4InitWithRenderTargetSize;
	static PVR_GetTrackedPose       f_PVR_GetTrackedPose;
	static PVR_GetRenderTargetSize  f_PVR_GetRenderTargetSize;
	static SetTextureFromUnityATW	f_SetTextureFromUnityATW;
	static PVR_IsHmdConnected		f_PVR_IsHmdConnected;
	static UE4Shutdown				f_UE4Shutdown;
	static PVR_Shutdown				f_PVR_Shutdown;
	static UE4Initialization        f_UE4Initialization;

	static bool Initialize();
	static bool GetIsInitialized();
	static void Uninitialize(void* DllHandle);

private:
	static HMODULE DllModule;
	static bool m_bIsInitialized;
	PicoPlugin();
	~PicoPlugin();
};

