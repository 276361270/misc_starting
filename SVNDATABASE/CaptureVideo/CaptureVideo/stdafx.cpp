// stdafx.cpp : ֻ������׼�����ļ���Դ�ļ�
// CaptureVideo.pch ����ΪԤ����ͷ
// stdafx.obj ������Ԥ����������Ϣ

#include "stdafx.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")


#ifdef DEBUG
#pragma comment(lib,"cv210d.lib")
#pragma comment(lib,"cxcore210d.lib")
#pragma comment(lib,"highgui210d.lib")
#else
#pragma comment(lib,"cv210.lib")
#pragma comment(lib,"cxcore210.lib")
#pragma comment(lib,"highgui210.lib")
#endif

#pragma comment(lib,"libmx")
#pragma comment(lib,"libmat")
#pragma comment(lib,"libeng")

#pragma comment(lib,"brook")