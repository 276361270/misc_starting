//-------------------------------------------------------------------
// CCaptureClass��Ƶ��׽��ʵ���ļ�CaptureVideo.cpp
//-------------------------------------------------------------------
// CaptureVideo.cpp: implementation of the CCaptureClass class.
//
/////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CaptureClass.h"
#include "Resource.h"
#include "SmartTee.h"
#include "HQGrabber.h"
#include "NullRenderer.h"
#include "AVIDecompressor.h"
#include "VideoRenderer.h"

#include "stdint.h"
#include "x264.h"

#pragma comment(lib,"..\\..\\..\\chapter15\\x264-060308\\build\\win32\\bin\\libx264")

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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int nCount=0;
x264_param_t param;
x264_t *h=NULL;
x264_picture_t pic;

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

	x264_param_t param;


	
}
CCaptureClass::~CCaptureClass()
{
	//
	fclose(fp);

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


/*���ò�����Ƶ���ļ�����ʼ��׽��Ƶ����д�ļ�*/
HRESULT CCaptureClass::CaptureImages(CDialog* pwnd)
{
	HRESULT hr=0;
	
	CClientDC dc(pwnd);
	
	HDC hdc=dc.GetSafeHdc();
	
	
// 	// ��ֹͣ��Ƶ
// 	m_pMC->Stop();
// 	// �����ļ�����ע��ú����ĵڶ�������������
// 	hr = m_pCapture->SetOutputFileName(&MEDIASUBTYPE_Avi, inFileName.AllocSysString(), &pMux, NULL );
//     // ��Ⱦý�壬���������˲���
// 	hr = m_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pBF, NULL, pMux );
// 	pMux->Release();
// 	// �ظ���Ƶ
// 	m_pMC->Run();

	BITMAPINFOHEADER bitInfoHeader;
	m_pHQGrabber->GetBitmapInfoHead(&bitInfoHeader);

	int nWidth=bitInfoHeader.biWidth;
	int nHeight=bitInfoHeader.biHeight;
	int nBiCount=bitInfoHeader.biBitCount;

	BYTE * pImageRGB=new BYTE[nWidth*nHeight*nBiCount/8];
//	BYTE * pImageYUV=new BYTE[nWidth*nHeight*3/2];

	m_pHQGrabber->Snapshot(pImageRGB,TRUE);


	uint8_t *puv=new uint8_t[nWidth*nHeight/2];


	nCount=0;
	for(int i=0;i<nHeight;i++)
	{
		for(int j=0;j<nWidth;j++)
		{
			BYTE *rColor=pImageRGB + i*nWidth*nBiCount/8 + j*nBiCount/8 + 0;
			BYTE *gColor=rColor+1;
			BYTE *bColor=rColor+2;

			BYTE yColor=(BYTE)(*rColor*0.299 + *gColor*0.587 + *bColor*0.114);
			BYTE uColor=(BYTE)(*rColor*0.147 - *gColor*0.289 + *bColor*0.436);
			BYTE vColor=(BYTE)(-*rColor*0.615 - *gColor*0.515 - *bColor*0.100);

// 			BYTE * temp=pImageYUV + i*nWidth*3 + j*3 + 0;
// 			*temp=yColor;
// 			*(temp+1)=uColor;
// 			*(temp+2)=vColor;
			//pic
			*(pic.img.plane[0]+nHeight*i+j)=yColor;
			if(j%2==0)
			{
				
				puv[nCount]=vColor;
				nCount++;
			}

		}
	}

	//memcpy(pic.img.plane[1],puv,nWidth*nHeight/2);

	delete []puv;
	
	//////////////////////////////////////////////////////////////////////////

	
// 	x264_picture_t pic_out;
// 	x264_nal_t *nal=NULL;
// 	int i_nal=0;
// 	x264_encoder_encode(h,&nal,&i_nal,&pic,&pic_out);
// 	uint8_t *data=new uint8_t[3000000];
// 
// 	for(int i = 0; i < i_nal; i++ )
// 	{
// 		int i_size=0;
// 		int i_data=0;
// 
// 		i_data = 3000000;
// 		if( ( i_size = x264_nal_encode( data, &i_data, 1, &nal[i] ) ) > 0 )
// 		{
// 			/*i_file += p_write_nalu( hout, data, i_size );*/
// 			fwrite(data,i_size,1,fp);
// 
// 		}
// 		else if( i_size < 0 )
// 		{
// 			fprintf( stderr, "need to increase buffer size (size=%d)\n", -i_size );
// 		}
// 	}


//	x264_picture_clean(&pic);
//	x264_picture_clean(&pic_out);

	///////////////////////////////////////////////////////////////////////////
	
	StretchDIBits(hdc,
		450,0,300,300,
		0,0,nWidth,nHeight,
		pImageRGB,(BITMAPINFO *)&bitInfoHeader,
		DIB_RGB_COLORS,SRCCOPY);

// 	StretchDIBits(hdc,
// 		0,0,rc.right-rc.left,rc.bottom-rc.top,
// 		0,0,nWidth,nHeight,
// 		pImageYUV,(BITMAPINFO *)&bitInfoHeader,
// 		DIB_PAL_COLORS,SRCCOPY);
	
	delete []pImageRGB;
//	delete []pImageYUV;
//	delete []data;

	
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

	x264_param_default(&param);
	param.i_width=640;
	param.i_height=480;
	
	
	h=x264_encoder_open(&param);
	param.i_width=640;
	param.i_height=480;
	x264_picture_alloc(&pic,X264_CSP_I420,param.i_width,param.i_height);

	fp=fopen("f:\\demo.264","wb");





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

