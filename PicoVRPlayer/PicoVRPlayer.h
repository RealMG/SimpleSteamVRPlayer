
// PicoVRPlayer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CPicoVRPlayerApp: 
// �йش����ʵ�֣������ PicoVRPlayer.cpp
//

class CPicoVRPlayerApp : public CWinApp
{
public:
	CPicoVRPlayerApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPicoVRPlayerApp theApp;