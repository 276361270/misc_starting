
// baizeDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CbaizeDlg �Ի���
class CbaizeDlg : public CDialog
{
// ����
public:
	CbaizeDlg(CWnd* pParent = NULL);	// ��׼���캯��
    ~CbaizeDlg(){ 
        if(m_pdata){
            free(m_pdata);
            m_pdata = NULL;
        }
    }

// �Ի�������
	enum { IDD = IDD_BAIZE_DIALOG };

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
    CStatic m_show;
    afx_msg void OnBnClickedOk();
    
    char*   m_pdata;
};
