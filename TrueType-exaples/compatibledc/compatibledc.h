
// compatibledc.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CcompatibledcApp:
// �йش����ʵ�֣������ compatibledc.cpp
//

class CcompatibledcApp : public CWinAppEx
{
public:
	CcompatibledcApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CcompatibledcApp theApp;