//-------------------------------------------------------------------
// CCaptureClass��Ƶ��׽��ʵ���ļ�CaptureVideo.cpp
//-------------------------------------------------------------------
// CaptureVideo.cpp: implementation of the CCaptureClass class.
//
/////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CaptureClass.h"
#include "SmartTee.h"
#include "HQGrabber.h"
#include "NullRenderer.h"
#include "AVIDecompressor.h"
#include "VideoRenderer.h"
#include "Resource.h"

#include "stdint.h"
#include "x264.h"
#include "avcodec.h"

#pragma comment(lib,"..\\..\\chapter15\\x264-060308\\build\\win32\\bin\\libx264")
#pragma comment(lib,"..\\..\\chapter15\\ffmpeg_h264_vc\\Debug\\H264Dec.lib")
#pragma comment(lib,"cscc")

class ColorSpaceConversions {

public:

	ColorSpaceConversions();

	void RGB24_to_YV12(unsigned char * in, unsigned char * out,int w, int h);
	void YV12_to_RGB24(unsigned char *src0,unsigned char *src1,unsigned char *src2,unsigned char *dst_ori,int width,int height);
	void YVU9_to_YV12(unsigned char * in,unsigned char * out, int w, int h);
	void YUY2_to_YV12(unsigned char * in,unsigned char * out, int w, int h);
	void YV12_to_YVU9(unsigned char * in,unsigned char * out, int w, int h);
	void YV12_to_YUY2(unsigned char * in,unsigned char * out, int w, int h);
};

#define CLIP(color) (unsigned char)((color>0xFF)?0xff:((color<0)?0:color))


extern "C" void		x264_param_default( x264_param_t * );
extern "C" int		x264_nal_encode( void *, int *, int b_annexeb, x264_nal_t *nal );
extern "C" int		x264_nal_decode( x264_nal_t *nal, void *, int );
extern "C" x264_t*	x264_encoder_open   ( x264_param_t * );
extern "C" int		x264_encoder_reconfig( x264_t *, x264_param_t * );
extern "C" int		x264_encoder_headers( x264_t *, x264_nal_t **, int * );
extern "C" int		x264_encoder_encode ( x264_t *, x264_nal_t **, int *, x264_picture_t *, x264_picture_t * );
extern "C" void		x264_encoder_close  ( x264_t * );
extern "C" void		x264_picture_alloc( x264_picture_t *pic, int i_csp, int i_width, int i_height );
extern "C" void		x264_picture_clean( x264_picture_t *pic );


void avcodec_init(void);
void avcodec_register_all(void);
AVCodec *avcodec_find_decoder(enum CodecID id);
AVCodecContext *avcodec_alloc_context(void);

int avcodec_open(AVCodecContext *avctx, AVCodec *codec);
AVFrame *avcodec_alloc_frame(void);
int avcodec_decode_video(AVCodecContext *avctx, AVFrame *picture, 
						 int *got_picture_ptr,
						 uint8_t *buf, int buf_size);
int avcodec_close(AVCodecContext *avctx);
void av_free(void *ptr);

FILE *outf;


#ifndef safedel
#define safedel(x) \
	if((x)) \
	{ \
	delete (x);\
	(x) = NULL;\
	}
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


x264_param_t param;

x264_t *h=NULL;
x264_picture_t pic;
AVCodec *codec;			  // Codec
AVCodecContext *c;		  // Codec Context
AVFrame *picture;		  // Frame




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCaptureClass::CCaptureClass()
{
	// COM ���ʼ��
	CoInitialize(NULL);
	m_hWnd = NULL;
	m_pVW = NULL;
	m_pMC = NULL;
	m_pGraph = NULL;
	m_pBF = NULL;
	m_pCapture = NULL; 

	m_pSmartTee=NULL;
	m_pHQGrabber=NULL;
	m_pNullRenderer=NULL;
	m_pAVIDecompressor=NULL;
	m_pVideoRenderer=NULL;

	x264_param_default(&param);
	param.i_width=640;
	param.i_height=480;
	h=x264_encoder_open(&param);
	x264_picture_alloc(&pic,X264_CSP_I420,param.i_width,param.i_height);


	fp=fopen("c:\\demo.264","wb");
	outf=fopen("c:\\demo.yuv","wb");

	avcodec_init(); 
	avcodec_register_all(); 
	codec = avcodec_find_decoder(CODEC_ID_H264);

	c = avcodec_alloc_context(); 
	avcodec_open(c, codec);
	picture   = avcodec_alloc_frame();
}
CCaptureClass::~CCaptureClass()
{
	//
	if(m_pMC)
		m_pMC->Stop();
	if(m_pVW)
	{
		m_pVW->put_Visible(OAFALSE);
		m_pVW->put_Owner(NULL);
	}

	safedel(m_pVideoRenderer);
	safedel(m_pAVIDecompressor);
	safedel(m_pNullRenderer);
	safedel(m_pHQGrabber);
	safedel(m_pSmartTee);

	srelease(m_pCapture);
	srelease(m_pMC);
	srelease(m_pGraph);
	srelease(m_pBF);

	fclose(fp);
	fclose(outf);

	CoUninitialize();
}

/*
ϵͳ�豸ö����Ϊ���ǰ�����ö����ע����ϵͳ�е�Fitler�ṩ��ͳһ�ķ������������ܹ����ֲ�ͬ��Ӳ���豸��������ͬһ��Filter֧�����ǡ������Щʹ��Windows����ģ�ͺ�KSProxy Filter���豸��˵�Ƿǳ����õġ�ϵͳ�豸ö���������ǰ���ͬ���豸ʵ�����жԴ�����ע����Ȼ����֧����ͬFilter����
��������������ϵͳ�豸ö������ѯ�豸��ʱ��ϵͳ�豸ö����Ϊ�ض����͵��豸���磬��Ƶ�������Ƶѹ����������һ��ö�ٱ�Enumerator��������ö������Category enumerator��Ϊÿ���������͵��豸����һ��Moniker������ö�����Զ���ÿһ�ּ��弴�õ��豸�������ڡ�
���������µĲ���ʹ��ϵͳ�豸ö������
����1�� ���÷���CoCreateInstance����ϵͳ�豸ö���������ʶ��CLSID��ΪCLSID_SystemDeviceEnum��
����2�� ����ICreateDevEnum::CreateClassEnumerator������������ö����������Ϊ����Ҫ�õ������͵�CLSID���÷�������һ��IEnumMoniker�ӿ�ָ�룬���ָ�������ͣ��ǿյģ��򲻴��ڣ�����ICreateDevEnum::CreateClassEnumerator������S_FALSE�����Ǵ�����룬ͬʱIEnumMonikerָ�루��ע��ͨ���������أ�Ҳ�ǿյģ����Ҫ�������ڵ���CreateClassEnumerator��ʱ����ȷ��S_OK���бȽ϶�����ʹ�ú�SUCCEEDED��
����3�� ʹ��IEnumMoniker::Next�������εõ�IEnumMonikerָ���е�ÿ��moniker���÷�������һ��IMoniker�ӿ�ָ�롣��Next����ö�ٵĵײ������ķ���ֵ��Ȼ��S_FALSE��������������Ҫ��S_OK�����м��顣
����4�� ��Ҫ�õ����豸��Ϊ�Ѻõ����ƣ�������Ҫ���û������н�����ʾ��������IMoniker::BindToStorage������
����5�� �����Ҫ���ɲ���ʼ��������豸��Filter����3����ָ���IMonitor::BindToObject����������������IFilterGraph::AddFilter�Ѹ�Filter��ӵ���ͼ�С�
*/

int CCaptureClass::EnumDevices(HWND hList)
{
	if (!hList)	return -1;
	int id = 0;

	//ö�ٲ����豸
	ICreateDevEnum *pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, 
						CLSCTX_INPROC_SERVER,
						IID_ICreateDevEnum,
						(void**)&pCreateDevEnum);

	if (hr != NOERROR) 	return -1;
	
	IEnumMoniker *pEm;
	//������Ƶ����ö����
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEm, 0);
	//������Ƶ������
	//hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory,&pEm, 0);

	if (hr != NOERROR) return -1;
	//����ö������λ
	pEm->Reset();
	ULONG cFetched;
	IMoniker *pM;
	while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
	{
		IPropertyBag *pBag;
		//�豸����ҳ
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr)) 
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR) 
			{
				id++;
				::SendMessage(hList, CB_ADDSTRING, 0,(LPARAM)var.bstrVal);
				SysFreeString(var.bstrVal);
			}
			pBag->Release();
		}
		pM->Release();
	}
	return id;
}

void CCaptureClass::Create()
{
	m_pSmartTee=new CSmartTee(m_pGraph);
	m_pHQGrabber=new CHQGrabber(m_pGraph);
	m_pNullRenderer=new CNullRenderer(m_pGraph);
	m_pAVIDecompressor=new CAVIDecompressor(m_pGraph);
	m_pVideoRenderer=new CVideoRenderer(m_pGraph);

	m_pSmartTee->CreateFilter();
	m_pHQGrabber->CreateFilter();
	m_pNullRenderer->CreateFilter();
	m_pAVIDecompressor->CreateFilter();
	m_pVideoRenderer->CreateFilter();
}

/*��ʼԤ����Ƶ����*/
HRESULT CCaptureClass::PreviewImages(int iDeviceID, HWND hWnd)
{
	HRESULT hr;
	
	// ��ʼ����Ƶ�����˲������������
	hr = InitCaptureGraphBuilder();
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Failed to get video interfaces!"));
		return hr;
	}

	// ��ָ���ɼ��豸���˲�������
	if(!BindFilter(iDeviceID, &m_pBF))
		return S_FALSE;
	// ���˲�����ӵ��˲���������
	hr = m_pGraph->AddFilter(m_pBF, L"Capture Filter");
	
	// ��Ⱦý�壬���������˲�����������
    hr = m_pCapture->RenderStream( &PIN_CATEGORY_PREVIEW, 
									0, 
									m_pBF, 
									NULL, 
									NULL );

	if( FAILED( hr ) )
	{
		AfxMessageBox(_T("Can��t build the graph"));
		return hr;
	}

	NukeDownstream(m_pBF);

	Create();

	IPin *pPin=FindPin(m_pBF,"����");

	IPin * pInputPinU1=m_pSmartTee->GetInputPin();
	pPin->Connect(pInputPinU1,0);

	m_pSmartTee->GetPreviewPin()->Connect(m_pAVIDecompressor->GetInputPin(),0);
	m_pAVIDecompressor->GetOutputPin()->Connect(m_pHQGrabber->GetInputPin(),0);
	m_pHQGrabber->GetOutputPin()->Connect(m_pVideoRenderer->GetInputPin(),0);



	//������Ƶ��ʾ����
	m_hWnd = hWnd; 
	SetupVideoWindow();
	//test for config
	hr = m_pMC->Run();
	if(FAILED(hr))
	{
		AfxMessageBox(_T("Couldn��t run the graph!"));
		return hr;
	}
	return S_OK;
}


IPin * CCaptureClass::FindPin(IBaseFilter *inFilter, char *inFilterName)
{
	HRESULT hr;
	IPin *pin=NULL;
	IPin * pFoundPin=NULL;
	IEnumPins *pEnumPins=NULL;
	hr=inFilter->EnumPins(&pEnumPins);
	if(FAILED(hr))
		return pFoundPin;
	ULONG fetched=0;
	while(SUCCEEDED(pEnumPins->Next(1,&pin,&fetched)) && fetched)
	{
		PIN_INFO pinfo;
		pin->QueryPinInfo(&pinfo);
		pinfo.pFilter->Release();
		CString str(pinfo.achName);
		if(str==CString(inFilterName))
		{
			pFoundPin=pin;
			pin->Release();
			break;
		}
		pin->Release();
	}
	pEnumPins->Release();
	return pFoundPin;
}

void CCaptureClass::NukeDownstream(IBaseFilter * inFilter)
{
	if(!m_pGraph || !inFilter)
		return ;
	IEnumPins *pPinEnum=NULL;

	if(SUCCEEDED(inFilter->EnumPins(&pPinEnum)))
	{
		pPinEnum->Reset();

		IPin * pin=NULL;
		unsigned long ulFetched=0;

		while(SUCCEEDED(pPinEnum->Next(1,&pin,&ulFetched)) && ulFetched)
		{
			if(pin)
			{
				IPin * pConnectedPin=NULL;
				pin->ConnectedTo(&pConnectedPin);
				if(pConnectedPin)
				{
					PIN_INFO pininfo;
					if(SUCCEEDED(pConnectedPin->QueryPinInfo(&pininfo)))
					{
						pininfo.pFilter->Release();
						if(pininfo.dir==PINDIR_INPUT)
						{
							NukeDownstream(pininfo.pFilter);
							m_pGraph->Disconnect(pConnectedPin);
							m_pGraph->Disconnect(pin);
							m_pGraph->RemoveFilter(pininfo.pFilter);
						}
					}
					pConnectedPin->Release();
				}
				pin->Release();
			}
		}
		pPinEnum->Release();
	}
}


uint8_t temp;
/*���ò�����Ƶ���ļ�����ʼ��׽��Ƶ����д�ļ�*/
HRESULT CCaptureClass::CaptureImages(CString inFileName,HDC hdc,RECT rc)
{
	HRESULT hr=0;

	BITMAPINFOHEADER bitInfoHeader;
	m_pHQGrabber->GetBitmapInfoHead(&bitInfoHeader);

	int nWidth=bitInfoHeader.biWidth;
	int nHeight=bitInfoHeader.biHeight;
	int nBiCount=bitInfoHeader.biBitCount;

	BYTE * pImageRGB32=new BYTE[nWidth*nHeight*4];
	BYTE * pImageRGB24=new BYTE[nWidth*nHeight*3];

	m_pHQGrabber->Snapshot(pImageRGB32,TRUE);

	for(int i=0;i<nHeight;i++)
	{
		for(int j=0;j<nWidth;j++)
		{
			LPBYTE pim=pImageRGB24+(i*nWidth+j)*3;
			LPBYTE pim2=pImageRGB32+(i*nWidth+j)*nBiCount/8;
			memcpy(pim,pim2,3);
		}
	}

	BYTE upreColor=0;
	BYTE vpreColor=0;

	BYTE *py=pic.img.plane[0];
	BYTE *pu=pic.img.plane[1];
	BYTE *pv=pic.img.plane[2];
	for(int i=nHeight-1;i>=0;i--)
	{
		for(int j=0;j<nWidth;j++)
		{
			BYTE *rColor=pImageRGB32 + i*nWidth*nBiCount/8 + j*nBiCount/8 + 0;
			BYTE *gColor=rColor+1;
			BYTE *bColor=rColor+2;

			BYTE yColor=(BYTE)(*rColor*0.299 + *gColor*0.587 + *bColor*0.114+16);
			BYTE uColor=(BYTE)(*rColor*0.168 - *gColor*0.331 + *bColor*0.500+128);
			BYTE vColor=(BYTE)(*rColor*0.500 - *gColor*0.4187 - *bColor*0.0813+128);


			*py++=yColor;
			if(j%2==0)
			{
				if(j%4==0)
					*pu++=(upreColor>>4)<<4 | vColor>>4 ;
				else
					*pv++=(vpreColor & 0xf0) | uColor>>4 ;
			}

			upreColor=uColor;
			vpreColor=vColor;

		}
	}
	ColorSpaceConversions space;
//	space.RGB24_to_YV12(pImageRGB24,(BYTE *)pic.img.plane[0],nWidth,nHeight);

	x264_picture_t pic_out;
	x264_nal_t *nal=NULL;

	int i_nal=0;
	pic.i_type = X264_TYPE_AUTO;
	pic.i_qpplus1 = 0;

	if(x264_encoder_encode(h,&nal,&i_nal,&pic,&pic_out)<0)
	{
		TRACE("failed x264_encoder_encode\n");
	}

	uint8_t *pYUV=new uint8_t[nWidth*nHeight*3/2];
	uint8_t *ppYUV[4];
	ppYUV[0]=pYUV;
	ppYUV[1]=pYUV+nWidth*nHeight;
	ppYUV[2]=ppYUV[1]+nWidth*nHeight/4;
	uint8_t *data=new uint8_t[3000000];
	for(int i = 0; i < i_nal; i++ )
	{
		int i_size;
		int i_data;

		i_data = 3000000;
		if( ( i_size = x264_nal_encode( data, &i_data, 1, &nal[i] ) ) > 0 )
		{
			fwrite(data,i_size,1,fp);
		


			int  got_picture=0;
			int	 consumed_bytes=0; 
			int  nalLen=i_size;
			
			uint8_t *Buf = (unsigned char*)calloc ( 1000000, sizeof(uint8_t));
			consumed_bytes= avcodec_decode_video(c, picture, &got_picture, data, nalLen);

			int icount=0;
			if(consumed_bytes > 0)
			{
// 				memcpy(ppYUV[0],picture->data[0],nWidth*nHeight);
// 				memcpy(ppYUV[1],picture->data[1],nWidth*nHeight/2);
// 				memcpy(ppYUV[2],picture->data[2],nWidth*nHeight/2);
				for(icount=0; icount<c->height; icount++)
				{
					//fwrite(picture->data[0] + icount * picture->linesize[0], 1, c->width, outf);
					memcpy(ppYUV[0]+icount*nWidth,picture->data[0] + icount * picture->linesize[0],c->width);
				}
				for(icount=0; icount<c->height/2; icount++)
				{
					//fwrite(picture->data[1] + icount * picture->linesize[1], 1, c->width/2, outf);
					memcpy(ppYUV[1]+icount*nWidth/2,picture->data[1] + icount * picture->linesize[1],c->width/2);
				}
				for(icount=0; icount<c->height/2; icount++)
				{
					//fwrite(picture->data[2] + icount * picture->linesize[2], 1, c->width/2, outf);
					memcpy(ppYUV[2]+icount*nWidth/2,picture->data[2] + icount * picture->linesize[2],c->width/2);
				}
			}

		}
		else if( i_size < 0 )
		{
			fprintf( stderr, "need to increase buffer size (size=%d)\n", -i_size );
		}
	}

	
//	convert_yuv420_rgb(pYUV,pImageRGB,nWidth,nHeight,1,2);
	
	space.YV12_to_RGB24(ppYUV[0],ppYUV[1],ppYUV[2],pImageRGB24,nWidth,nHeight);


	bitInfoHeader.biBitCount=24;
	bitInfoHeader.biSizeImage=3*nWidth*nHeight;
	StretchDIBits(hdc,
		0,0,rc.right-rc.left,rc.bottom-rc.top,
		0,0,nWidth,nHeight,
		pImageRGB24,(BITMAPINFO *)&bitInfoHeader,
		DIB_RGB_COLORS,SRCCOPY);


	delete []pImageRGB32;
	delete []pImageRGB24;
	delete []pYUV;
//	delete []data;
//	x264_picture_clean(&pic);

//	fclose(fp);

	return hr;
}

// ��ָ���ɼ��豸���˲�������
bool CCaptureClass::BindFilter(int deviceId, IBaseFilter **pFilter)
{
	if (deviceId < 0) return false;

	// enumerate all video capture devices
	
	ICreateDevEnum *pCreateDevEnum;

	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, 
					CLSCTX_INPROC_SERVER,
					IID_ICreateDevEnum,
					(void**)&pCreateDevEnum);
	if (hr != NOERROR) return false;
	
	IEnumMoniker *pEm;

	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEm, 0);
	if (hr != NOERROR) return false;
	pEm->Reset();
	ULONG cFetched;
	IMoniker *pM;
	int index = 0;
	while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK, index <= deviceId)
	{
		IPropertyBag *pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr)) 
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR) 
			{
				if (index == deviceId)
				{
					pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pFilter);
				}
				SysFreeString(var.bstrVal);
			}			
			pBag->Release();
		}
		pM->Release();
		index++;
	}
	return true;
}

/* �����˲����������������ѯ����ֿ��ƽӿ� */
HRESULT CCaptureClass::InitCaptureGraphBuilder()
{
	HRESULT hr;

	// ����IGraphBuilder�ӿ�
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, 
						CLSCTX_INPROC_SERVER,
						IID_IGraphBuilder, (void **)&m_pGraph);
	if (FAILED(hr)) return hr;
	
	// ����ICaptureGraphBuilder2�ӿ�
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2 , NULL,
						CLSCTX_INPROC,
						IID_ICaptureGraphBuilder2, (void **)&m_pCapture);
	if (FAILED(hr)) return hr;

	// ��ʼ���˲������������IGraphBuilder
	m_pCapture->SetFiltergraph(m_pGraph);
	
	// ��ѯý����ƽӿ�
	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **)&m_pMC);
	if (FAILED(hr)) return hr;
	// ��ѯ��Ƶ���ڽӿ�
	hr = m_pGraph->QueryInterface(IID_IVideoWindow, (LPVOID *) &m_pVW);
	if (FAILED(hr)) return hr;

	return hr;

}

/* ������Ƶ��ʾ���ڵ����� */ 
HRESULT CCaptureClass::SetupVideoWindow()
{
	HRESULT hr;
	//ljz 
	hr = m_pVW->put_Visible(OAFALSE);
	
	hr = m_pVW->put_Owner((OAHWND)m_hWnd);
	if (FAILED(hr)) return hr;

	hr = m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
	if (FAILED(hr)) return hr;

	ResizeVideoWindow();
	hr = m_pVW->put_Visible(OATRUE);
	return hr;
}

/* ������Ƶ���ڴ�С */
void CCaptureClass::ResizeVideoWindow()
{
	if (m_pVW)
	{
		//��ͼ�������������
		CRect rc;
		::GetClientRect(m_hWnd,&rc);
		m_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
	} 
}

/* ���������˲��������ļ����Ա���ʹ��GraphEdit����鿴*/
void CCaptureClass::SaveGraph(TCHAR *wFileName)
{
	HRESULT hr;
	USES_CONVERSION;

	//CFileDialog dlg(TRUE);
	
	
	//if (dlg.DoModal()==IDOK)
	{
		//WCHAR wFileName[MAX_PATH];
		//MultiByteToWideChar(CP_ACP, 0, dlg.GetPathName(), -1, wFileName, MAX_PATH);
			
		IStorage* pStorage=NULL;
		
		// First, create a document file that will hold the GRF file
		hr = ::StgCreateDocfile(A2OLE(wFileName), STGM_CREATE|STGM_TRANSACTED|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,	0, &pStorage);
		if (FAILED(hr))
		{
			AfxMessageBox(TEXT("Can not create a document"));
			return;
		}

		// Next, create a stream to store.
		WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
		IStream *pStream;

		hr = pStorage->CreateStream(wszStreamName,STGM_WRITE|STGM_CREATE|STGM_SHARE_EXCLUSIVE, 0, 0, &pStream);

		if(FAILED(hr))
		{
			AfxMessageBox(TEXT("Can not create a stream"));
			pStorage->Release();
			return;
		}

		// The IpersistStream::Save method converts a stream
		// into a persistent object.
		IPersistStream *pPersist = NULL;

		m_pGraph->QueryInterface(IID_IPersistStream,  reinterpret_cast<void**>(&pPersist));
		hr = pPersist->Save(pStream, TRUE);
		pStream->Release();
		pPersist->Release();

		if(SUCCEEDED(hr))
		{
			hr = pStorage->Commit(STGC_DEFAULT);
			if (FAILED(hr))
			{
			AfxMessageBox(TEXT("can not store it"));
			}
		}
		pStorage->Release();
	}

}

/*��������ͷ����Դ��ʽ���ֱ��ʡ�RGB/I420��*/
void CCaptureClass::ConfigCameraPin(HWND hwndParent)
{
    HRESULT hr;
	IAMStreamConfig *pSC;
    ISpecifyPropertyPages *pSpec;

	//ֻ��ֹͣ�󣬲��ܽ���pin���Ե�����
	m_pMC->Stop();

	hr = m_pCapture->FindInterface(&PIN_CATEGORY_CAPTURE,
                        &MEDIATYPE_Video, m_pBF,
                        IID_IAMStreamConfig, (void **)&pSC);

    CAUUID cauuid;

	hr = pSC->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
	if(hr == S_OK)
	{
		hr = pSpec->GetPages(&cauuid);
		//��ʾ����ҳ
		hr = OleCreatePropertyFrame(hwndParent,	30, 30, NULL, 1, 
						(IUnknown **)&pSC, cauuid.cElems,
						(GUID *)cauuid.pElems, 0, 0, NULL);
		
		//�ͷ��ڴ桢��Դ
  		CoTaskMemFree(cauuid.pElems);
		pSpec->Release();
		pSC->Release();
	}
	//�ظ�����
	m_pMC->Run();

}

/*����ͼ����������ȡ�ɫ�ȡ����Ͷȵ�*/
void CCaptureClass::ConfigCameraFilter(HWND hwndParent)
{
	HRESULT hr=0;

	ISpecifyPropertyPages *pProp;

	hr = m_pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);

	if (SUCCEEDED(hr)) 
	{
		// ��ȡ�˲������ƺ�IUnknown�ӿ�ָ��
		FILTER_INFO FilterInfo;
		hr = m_pBF->QueryFilterInfo(&FilterInfo); 
		IUnknown *pFilterUnk;
		m_pBF->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// ��ʾ��ҳ
		CAUUID caGUID;
		pProp->GetPages(&caGUID);

		OleCreatePropertyFrame(
			hwndParent,				// ������
			0, 0,                   // Reserved
			FilterInfo.achName,     // �Ի������
			1,                      // ���˲�����Ŀ����Ŀ
			&pFilterUnk,            // Ŀ��ָ������ 
			caGUID.cElems,          // ����ҳ��Ŀ
			caGUID.pElems,          // ����ҳ��CLSID����
			0,                      // ���ر�ʶ
			0, NULL                 // Reserved
		);

		// �ͷ��ڴ桢��Դ
		CoTaskMemFree(caGUID.pElems);
		pFilterUnk->Release();
		FilterInfo.pGraph->Release(); 
		pProp->Release();
	}
	//m_pMC->Run();
}


void CCaptureClass::convert_yuv420_rgb(unsigned char * src, unsigned char * dst,
									   int width, int height, int flipUV, int ColSpace)
{
	unsigned char *Y, *U, *V ;
	int y1, y2, u, v ;
	int v1, v2, u1, u2 ;
	unsigned char *pty1, *pty2 ;
	int i, j ;
	unsigned char *RGB1, *RGB2 ;
	int r, g, b ;

	//Initialization
	Y = src;
	V = Y + width * height;
	U = Y + width * height + width * height / 4;

	pty1 = Y;
	pty2 = pty1 + width;
	RGB1 = dst;
	RGB2 = RGB1 + 3 * width;
	for (j = 0; j < height; j += 2) {
		for (i = 0; i < width; i += 2) {
			if (flipUV) {
				u = (*V++) - 128;
				v = (*U++) - 128;
			} else {
				v = (*V++) - 128;
				u = (*U++) - 128;
			}

			switch (ColSpace) {
		  case 0:
			  {
				  // M$ color space
				  v1 = ((v << 10) + (v << 9) + (v << 6) + (v << 5)) >> 10;    // 1.593
				  u1 = ((u << 8) + (u << 7) + (u << 4)) >> 10;    //         0.390
				  v2 = ((v << 9) + (v << 4)) >> 10;    //                0.515
				  u2 = ((u << 11) + (u << 4)) >> 10;    //               2.015
			  }
			  break;
			  // PAL specific
		  case 1:
			  {
				  v1 = ((v << 10) + (v << 7) + (v << 4)) >> 10;    //      1.1406
				  u1 = ((u << 8) + (u << 7) + (u << 4) + (u << 3)) >> 10;    // 0.3984
				  v2 = ((v << 9) + (v << 6) + (v << 4) + (v << 1)) >> 10;    // 0.5800
				  u2 = ((u << 11) + (u << 5)) >> 10;    //              2.0312
			  }
			  break;
			  // V4l2
		  case 2:
			  {
				  v1 = ((v << 10) + (v << 8) + (v << 7) + (v << 5)) >> 10;    //       1.406
				  u1 = ((u << 8) + (u << 6) + (u << 5)) >> 10;    //                0.343
				  v2 = ((v << 9) + (v << 7) + (v << 6) + (v << 5)) >> 10;    //        0.718
				  u2 = ((u << 10) + (u << 9) + (u << 8) + (u << 4) + (u << 3)) >> 10;    // 1.773
			  }
			  break;
		  case 3:
			  {
				  v1 = u1 = v2 = u2 = 0;
			  }
			  break;
		  default:
			  break;
			} // end switch 
			//up-left

			y1 = (*pty1++);
			if (y1 > 0) {
				r = y1 + (v1);
				g = y1 - (u1) - (v2);
				b = y1 + (u2);

				r = CLIP (r);
				g = CLIP (g);
				b = CLIP (b);
			} else {
				r = g = b = 0;
			}
			/*       *RGB1++ = r; */
			/*       *RGB1++ = g; */
			/*       *RGB1++ = b; */
			*RGB1++ = b;
			*RGB1++ = g;
			*RGB1++ = r;

			//down-left
			y2 = (*pty2++);
			if (y2 > 0) {
				r = y2 + (v1);
				g = y2 - (u1) - (v2);
				b = y2 + (u2);

				r = CLIP (r);
				g = CLIP (g);
				b = CLIP (b);


			} else {
				r = b = g = 0;
			}
			/*       *RGB2++ = r; */
			/*       *RGB2++ = g; */
			/*       *RGB2++ = b; */
			*RGB2++ = b;
			*RGB2++ = g;
			*RGB2++ = r;

			//up-right
			y1 = (*pty1++);
			if (y1 > 0) {
				r = y1 + (v1);
				g = y1 - (u1) - (v2);
				b = y1 + (u2);

				r = CLIP (r);
				g = CLIP (g);
				b = CLIP (b);
			} else {
				r = g = b = 0;
			}
			*RGB1++ = b;
			*RGB1++ = g;
			*RGB1++ = r;

			/*       *RGB1++ = r; */
			/*       *RGB1++ = g; */
			/*       *RGB1++ = b; */
			//down-right
			y2 = (*pty2++);
			if (y2 > 0) {
				r = y2 + (v1);
				g = y2 - (u1) - (v2);
				b = y2 + (u2);

				r = CLIP (r);
				g = CLIP (g);
				b = CLIP (b);
			} else {
				r = b = g = 0;
			}

			/*       *RGB2++ = r; */
			/*       *RGB2++ = g; */
			/*       *RGB2++ = b; */

			*RGB2++ = b;
			*RGB2++ = g;
			*RGB2++ = r;

		}
		RGB1 += 3 * width;
		RGB2 += 3 * width;
		pty1 += width;
		pty2 += width;
	}

}
