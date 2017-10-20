
// AACEncoderManager.cpp: implementation of the CAACEncoderManager class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "lc_faac.h"
#include <process.h>

#pragma comment(lib,"libfaac.lib")

lc_faac_encoder::lc_faac_encoder()
	:isWork_(false)
	,hEventStop(NULL)
	,hEventReady(NULL)
{
	hEventStop = CreateEvent(NULL,FALSE,FALSE,NULL);
	hEventReady = CreateEvent(NULL,FALSE,FALSE,NULL);

	InitializeCriticalSection(&m_PreSection); 
	InitializeCriticalSection(&m_HaveSection);
	m_nSampleRate=16000;//44100;  // 采样率
    m_nChannels=2;         // 声道数
    m_nBits=16;      // 单样本位数
  //最大输入样本数
    m_nMaxOutputBytes=0;	//最大输出字节
	
	m_hfaac=NULL;
    m_pfaacconf=NULL; 

	m_pfaacbuffer=NULL;
	m_pfaacbuffer=(unsigned char*)malloc(4096);

	m_pfaacinfobuffer=NULL;//(unsigned char*)malloc(100);
}

lc_faac_encoder::~lc_faac_encoder()
{
	DeleteCriticalSection(&m_PreSection);
	DeleteCriticalSection(&m_HaveSection);
	Clean();
	if(m_pfaacbuffer)
	{
		free(m_pfaacbuffer);
		m_pfaacbuffer=NULL;
	}
}

unsigned char* lc_faac_encoder::Encoder(unsigned char*indata,int inlen,int &outlen)
{
	if(m_hfaac!=NULL)
	{
		int nInputSamples = inlen/2;
		outlen = faacEncEncode(m_hfaac, (int32_t*)indata, nInputSamples, m_pfaacbuffer, 4096);
	}
	else
	{
		outlen=-1;
	}
	if (outlen>0)
	{
		return m_pfaacbuffer;
	}
	else
	{
		return NULL;
	}
	return NULL;
}

PDT lc_faac_encoder::getPdt()
{
	AutoLock autolock(&m_HaveSection);
	PDT pdt = m_vecHaveEncode.front();
	m_vecHaveEncode.erase(m_vecHaveEncode.begin());
	return pdt;
}

int64_t lc_faac_encoder::getFirstFrameTime()
{
	AutoLock autolock(&m_HaveSection);
	if (m_vecHaveEncode.empty())
	{
		return 0;
	}
	else
	{
		return m_vecHaveEncode.front().timeTicket;
	}
}

void lc_faac_encoder::EncoderProcess()
{

	HANDLE waitArray[2] = { hEventStop, hEventReady };
	while (true)
	{   
		DWORD result = WaitForMultipleObjects(2, waitArray, FALSE, 1000); 
		if (result == WAIT_OBJECT_0)
		{
			break;
		}
		else if (result == WAIT_OBJECT_0 + 1 || result == WAIT_TIMEOUT)
		{
			while(true)
			{
				PDT Waitpdt;
				{
					AutoLock autolock(&m_PreSection);
					if (m_vecWaitEncode.empty())
						break;
					std::vector<PDT>::iterator it = m_vecWaitEncode.begin();
					Waitpdt = *it;
					m_vecWaitEncode.erase(it);
				}

				
				void* pBuffer = NULL;
				int szBuffer = 0;

				pBuffer = Encoder((unsigned char*)Waitpdt.pbuffer,Waitpdt.buffersize,szBuffer);
				if (szBuffer == 0)
					continue;

				PDT pdtEncode;
				pdtEncode.buffersize = szBuffer;
				pdtEncode.timeTicket = Waitpdt.timeTicket;
				pdtEncode.pbuffer = malloc(szBuffer);
				memcpy(pdtEncode.pbuffer,pBuffer,szBuffer);

				//释放内存
				Waitpdt.release();
				{
					AutoLock autolock(&m_HaveSection);
					m_vecHaveEncode.push_back(pdtEncode);
				}
			}
		}  
	}
}


unsigned int WINAPI lc_faac_encoder::EncodeThread(void* param)
{
	lc_faac_encoder* pThis = (lc_faac_encoder*)param;
	if (pThis)
	{
		pThis->EncoderProcess();
	}

	return 0;
}

int lc_faac_encoder::Init(int nSampleRate, int nChannels)
{
	if (isWork_)
	{
		return 0;
	}

	m_nSampleRate = nSampleRate;
	m_nChannels = nChannels;
	m_nMaxOutputBytes=(6144/8)*m_nChannels;	
	m_hfaac = faacEncOpen(m_nSampleRate, m_nChannels, &m_nInputSamples, &m_nMaxOutputBytes);
	if (m_hfaac!=NULL)
	{	
		m_pfaacconf = faacEncGetCurrentConfiguration(m_hfaac);  
		m_pfaacconf->inputFormat = FAAC_INPUT_16BIT;
		m_pfaacconf->outputFormat=1;//0raw 1adst
		m_pfaacconf->useTns=true;
		m_pfaacconf->useLfe=false;
		m_pfaacconf->aacObjectType=LOW;
		m_pfaacconf->shortctl=SHORTCTL_NORMAL;
		m_pfaacconf->quantqual=100;
		m_pfaacconf->bandWidth=0;
		m_pfaacconf->bitRate=0;
		faacEncSetConfiguration(m_hfaac,m_pfaacconf);

		faacEncGetDecoderSpecificInfo(m_hfaac,&m_pfaacinfobuffer,&m_nfaacinfosize);
	}
	return 0;
}

void lc_faac_encoder::Start()
{
	CloseHandle((HANDLE)_beginthreadex(NULL, 0, &lc_faac_encoder::EncodeThread, (void*)this, 0, NULL));
}

void lc_faac_encoder::Stop()
{
	::SetEvent(hEventStop);
}

int lc_faac_encoder::Clean()
{
	if (m_hfaac!=NULL)
	{
		faacEncEncode(m_hfaac, NULL, 0, m_pfaacbuffer, 4096);
		faacEncClose(m_hfaac);
		m_hfaac=NULL;

	}
	return 0;
}

bool lc_faac_encoder::IsWorking(void)
{
	return m_hfaac!=NULL;
}

void lc_faac_encoder::AddEncoderData(const PDT& pdt)
{
	AutoLock autolock(&m_PreSection);
	m_vecWaitEncode.push_back(pdt);
	SetEvent(hEventReady);
}

lc_faac_encoder& lc_faac_encoder::get()
{
	static lc_faac_encoder s_instance;
	return s_instance;
}


