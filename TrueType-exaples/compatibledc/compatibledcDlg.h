
// compatibledcDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CcompatibledcDlg �Ի���
class CcompatibledcDlg : public CDialog
{
// ����
public:
	CcompatibledcDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_COMPATIBLEDC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    CStatic m_show;
    CButton m_ok;
    CButton m_cancle;
};
