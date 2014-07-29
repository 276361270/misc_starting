
// baizeDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "baize.h"
#include "baizeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CbaizeDlg �Ի���




CbaizeDlg::CbaizeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CbaizeDlg::IDD, pParent),
	m_pdata(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CbaizeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SHOW, m_show);
}

BEGIN_MESSAGE_MAP(CbaizeDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDOK, &CbaizeDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CbaizeDlg ��Ϣ�������

BOOL CbaizeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CbaizeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CbaizeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int set_point(char* pdata,int x,int y,int width,int height)
{
    char* p = (pdata + (( y) * width + x) * 3);
    *(p + 0) = 0;
    *(p + 1) = 0;
    *(p + 2) = 0;
    return 0;
}

class MPoint{
public:
    int x;
    int y;
    
public:
    MPoint(int _x,int _y){
        x = _x;
        y = _y;
    }
    
};

MPoint operator *(int t,MPoint point){
    point.x *= t;
    point.y *= t;
    return point;
}

MPoint operator +(MPoint p1,MPoint p2){
    p1.x += p2.x;
    p1.y += p2.y;
    return p1;
}

void CbaizeDlg::OnBnClickedOk()
{
    RECT rc = {0};
    m_show.GetWindowRect(&rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    
    BITMAPINFOHEADER info = {0};
    info.biBitCount = 24;
    info.biCompression = 0;
    info.biHeight = height;
    info.biWidth = width;
    info.biSize = sizeof(BITMAPINFOHEADER);
    info.biSizeImage = width * height * info.biBitCount / 8;
    info.biPlanes = 1;
    
    if(m_pdata == NULL){
        m_pdata = (char*)malloc(width * height * 3);
        memset(m_pdata, 0xff, width * height * 3);
    }
    
    MPoint point1(0,0);
    MPoint point2(width,height);
    
    int x = 0;
    int y = 0;
    const int factor = 1000;
    for(int t = 0;t <= factor; t++){
        MPoint tmp = (factor - t) * point1 + t * point2;
        x = tmp.x / factor;
        y = tmp.y / factor;
        x = max(0,min(x,width - 1));
        y = max(0,min(y,height - 1));
        set_point(m_pdata,x,y,width,height);
    }
    
    
    StretchDIBits(m_show.GetDC()->GetSafeHdc(),
                  0,0,width,height,
                  0,0,width,height,
                  m_pdata,(BITMAPINFO*)&info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}
