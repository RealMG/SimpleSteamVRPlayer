
// PicoVRPlayerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PicoVRPlayer.h"
#include "PicoVRPlayerDlg.h"
#include "afxdialogex.h"


#include "PicoImports.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPicoVRPlayerDlg 对话框


void CPicoVRPlayerDlg::ParseCommands(LPWSTR* argv, int argc)
{
	if (argc > 1)
	{

		m_sFilePath = argv + 1;
		for (int ii = 2; ii < argc; ++ii)
		{
			LPWSTR* addr = argv + ii;
			//wchar_t* addr1 = (wchar_t*)addr;
			if (wcscmp(*addr, L"SCALE") == 0)
			{
				isScaleChecked = 1;
			}
		}
	}
}



CPicoVRPlayerDlg::CPicoVRPlayerDlg(LPWSTR* argv, int argc, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PICOVRPLAYER_DIALOG, pParent)
	, isScaleChecked(0)
	, m_sFilePath(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	//m_sFilePath = fPath;
	ParseCommands(argv, argc);
}

void CPicoVRPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPicoVRPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CPicoVRPlayerDlg::OnBnClickedButtonPlay)
	//ON_BN_CLICKED(IDC_BUTTON_RESIZE, &CPicoVRPlayerDlg::OnBnClickedButtonResize)
	ON_WM_SIZE()
	ON_COMMAND(ID_OPEN_OPENFILE, &CPicoVRPlayerDlg::OnOpenOpenfile)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CPicoVRPlayerDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CPicoVRPlayerDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_CHECK_SCALE, &CPicoVRPlayerDlg::OnBnClickedCheckScale)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CPicoVRPlayerDlg 消息处理程序

BOOL CPicoVRPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//对话框默认最大化弹出
	//ShowWindow(SW_SHOWMAXIMIZED);


	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	{
		CScrollBar *scrollBar = (CScrollBar *)GetDlgItem(IDC_SCROLLBAR_VOL); // 取得控件的指针 
		if (scrollBar->GetSafeHwnd())
		{
			scrollBar->SetScrollRange(0, 100);
			// 设置水平滚动条的初始位置为20
			scrollBar->SetScrollPos(70);
		}


	}
	// TODO: 在此添加额外的初始化代码
	if (!FPicoPlayerImports::Initialize())
	{
		return FALSE;
	}

	GetClientRect(&m_LastRect);// 将对话框大小设为旧大小 


	if (m_sFilePath != nullptr)
	{


		CButton *checkButton = (CButton *)GetDlgItem(IDC_CHECK_SCALE); // 取得控件的指针 
		if (checkButton->GetSafeHwnd())
		{
			checkButton->SetCheck(isScaleChecked);
		}



		CWnd *pWnd = GetDlgItem(IDC_PICTURE); // 取得控件的指针  
		HWND hwnd = pWnd->GetSafeHwnd(); // 取得控件的句柄

		int ret = 0;
		ret = FPicoPlayerImports::f_Create(MovieType::LOCAL, 0);
		if (ret < 0)
		{
			return FALSE;
		}

		ret = FPicoPlayerImports::f_SetWindowHandle(hwnd);
		if (ret < 0)
		{
			return FALSE;
		}



		ret = FPicoPlayerImports::f_OpenMovie((*m_sFilePath), LR, 0);
		if (ret < 0)
		{
			return FALSE;
		}



		OnBnClickedCheckScale();

		FPicoPlayerImports::f_Play();
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPicoVRPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPicoVRPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CPicoVRPlayerDlg::OnClose()
{
	FPicoPlayerImports::f_Stop();
	FPicoPlayerImports::f_Destroy();
	//FPicoPlayerImports::Uninitialize();
	CDialogEx::OnClose();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPicoVRPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CPicoVRPlayerDlg::OnBnClickedButtonPlay()
{
	// TODO: 在此添加控件通知处理程序代码

	FPicoPlayerImports::f_Play();
}


void CPicoVRPlayerDlg::ChangeSize(CWnd *pWnd, int cx, int cy)
{



	if (pWnd)  //判断是否为空，因为对话框创建时会调用此函数，而当时控件还未创建   
	{
		CRect rect;   //获取控件变化前大小
		LONG cWidth, cHeight; //记录控件的右部到窗体右部的距离，记录控件的底部到窗体底部的距离
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);//将控件大小转换为在对话框中的区域坐标
		cWidth = m_LastRect.Width() - rect.right;
		cHeight = m_LastRect.Height() - rect.bottom;
		rect.left = cx - rect.Width() - cWidth;
		rect.right = cx - cWidth;
		rect.top = cy - rect.Height() - cHeight;
		rect.bottom = cy - cHeight;
		pWnd->MoveWindow(rect);//设置控件大小
	}
}


void CPicoVRPlayerDlg::OnSize(UINT nType, int cx, int cy)
{

	CDialogEx::OnSize(nType, cx, cy);
	// TODO: 在此处添加消息处理程序代码
	if (nType == 1) return;//最小化则什么都不做

	CWnd *pWnd = GetDlgItem(IDC_PICTURE); // 取得控件的指针 
	if (pWnd->GetSafeHwnd())
	{
		ChangeSize(pWnd, cx, cy);
	}

	pWnd = GetDlgItem(IDC_BUTTON_PLAY); // 取得控件的指针 
	if (pWnd->GetSafeHwnd())
	{
		ChangeSize(pWnd, cx, cy);
	}

	pWnd = GetDlgItem(IDC_BUTTON_PAUSE); // 取得控件的指针 
	if (pWnd->GetSafeHwnd())
	{
		ChangeSize(pWnd, cx, cy);
	}
	pWnd = GetDlgItem(IDC_BUTTON_STOP); // 取得控件的指针 
	if (pWnd->GetSafeHwnd())
	{
		ChangeSize(pWnd, cx, cy);
	}

	pWnd = GetDlgItem(IDC_CHECK_SCALE); // 取得控件的指针 
	if (pWnd->GetSafeHwnd())
	{
		ChangeSize(pWnd, cx, cy);
	}

	GetClientRect(&m_LastRect);// 将变化后的对话框大小设为旧大小     
}


void CPicoVRPlayerDlg::OnOpenOpenfile()
{
	// TODO: 在此添加命令处理程序代码

	//FPicoPlayerImports::f_Stop();
	//FPicoPlayerImports::f_Destroy();



	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, L"Videos(*.mp4;*.avi;*.mkv)|*.mp4;*.avi;*.mkv|All(*.*)|*.*||");
	CString csFileName;
	if (dlg.DoModal() == IDOK)
	{
		csFileName = dlg.GetPathName();    //选择的文件路径   


		CWnd *pWnd = GetDlgItem(IDC_PICTURE); // 取得控件的指针  
		HWND hwnd = pWnd->GetSafeHwnd(); // 取得控件的句柄
		CRect rectTemp;
		pWnd->GetWindowRect(rectTemp);
		ScreenToClient(&rectTemp);//将控件大小转换为在对话框中的区域坐标 


		OnBnClickedButtonStop();

		int ret = 0;
		ret = FPicoPlayerImports::f_Create(MovieType::LOCAL, 0);
		if (ret < 0)
		{
			return;
		}

		ret = FPicoPlayerImports::f_SetWindowHandle(hwnd);
		if (ret < 0)
		{
			return;
		}



		ret = FPicoPlayerImports::f_OpenMovie(csFileName.GetBuffer(), LR, 0);
		if (ret < 0)
		{
			return;
		}

		//rectTemp.SetRect( = FPicoPlayerImports::f_GetMovieWidth();

		//pWnd->MoveWindow();
		//FPicoPlayerImports::f_GetMovieWidth();

		OnBnClickedCheckScale();

		FPicoPlayerImports::f_Play();
	}

}


void CPicoVRPlayerDlg::OnBnClickedButtonPause()
{
	// TODO: 在此添加控件通知处理程序代码
	FPicoPlayerImports::f_Pause();
}


void CPicoVRPlayerDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码
	FPicoPlayerImports::f_Stop();
	FPicoPlayerImports::f_Destroy;
}




void CPicoVRPlayerDlg::OnBnClickedCheckScale()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton *checkButton = (CButton *)GetDlgItem(IDC_CHECK_SCALE); // 取得控件的指针 
	if (checkButton->GetSafeHwnd())
	{
		int checked = checkButton->GetCheck();
		//FPicoPlayerImports::f_SetScreenScale(checked);
	}

}


void CPicoVRPlayerDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	

	int pos = pScrollBar->GetScrollPos();

	switch (nSBCode)
	{
		// 如果向左滚动一列，则pos减1
	case SB_LINELEFT:
		pos -= 1;
		break;
		// 如果向右滚动一列，则pos加1
	case SB_LINERIGHT:
		pos += 1;
		break;
		// 如果向左滚动一页，则pos减10
	case SB_PAGELEFT:
		pos -= 10;
		break;
		// 如果向右滚动一页，则pos加10
	case SB_PAGERIGHT:
		pos += 10;
		break;
		// 如果滚动到最左端，则pos为1
	case SB_LEFT:
		pos = 0;
		break;
		// 如果滚动到最右端，则pos为100
	case SB_RIGHT:
		pos = 100;
		break;
		// 如果拖动滚动块到指定位置，则pos赋值为nPos的值
	case SB_THUMBPOSITION:
		pos = nPos;
		break;

	default:
		break;
	}
	pScrollBar->SetScrollPos(pos);
	FPicoPlayerImports::f_SetVolume(pos);

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}
