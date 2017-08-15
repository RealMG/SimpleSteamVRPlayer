
// PicoVRPlayerDlg.h : ͷ�ļ�
//

#pragma once


// CPicoVRPlayerDlg �Ի���
class CPicoVRPlayerDlg : public CDialogEx
{
// ����
public:
	CPicoVRPlayerDlg(LPWSTR* argv, int argc, CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PICOVRPLAYER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;
	LPWSTR* m_sFilePath;
	int isScaleChecked;
	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	void ParseCommands(LPWSTR* argv, int argc);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonPlay();
	void ChangeSize(CWnd *pWnd, int cx, int cy);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnOpenOpenfile();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonStop();
private:
	CRect m_LastRect;
public:

	afx_msg void OnBnClickedCheckScale();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
