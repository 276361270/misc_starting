
// keyboard.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CkeyboardApp:
// �йش����ʵ�֣������ keyboard.cpp
//

class CkeyboardApp : public CWinAppEx
{
public:
	CkeyboardApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CkeyboardApp theApp;