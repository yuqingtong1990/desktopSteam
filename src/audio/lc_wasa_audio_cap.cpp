#include "stdafx.h"
#include "lc_faac.h"
#include "lc_wasa_audio_cap.h"

#define REFERENCE_TIME_VAL 5 * 10000000
extern FILE* f;
extern CRITICAL_SECTION gSection;

/// pwfx->nSamplesPerSec    = 44100; 
/// 不支持修改采样率， 看来只能等得到数据之后再 swr 转换了 
BOOL AdjustFormatTo16Bits(WAVEFORMATEX *pwfx)
{
    BOOL bRet(FALSE);
    if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        pwfx->wFormatTag = WAVE_FORMAT_PCM;
        pwfx->wBitsPerSample = 16;
        pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
        pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

        bRet = TRUE;
    }
    else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
        if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
        {
            pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            pEx->Samples.wValidBitsPerSample = 16;
            pwfx->wBitsPerSample = 16;
            pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
            pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
            bRet = TRUE;
        }
    }

    return bRet;
}

lc_wasa_audio_cap::lc_wasa_audio_cap(DevType dev)
    :bInit_(false)
    ,m_devtype(dev)
    ,m_nDevIndex(-1)
    ,m_pcb(NULL)
	,filedump_(NULL)
{
    ::InitializeCriticalSection(&m_sectionDataCb);
    m_hCaptureReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hCaptureStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

lc_wasa_audio_cap::~lc_wasa_audio_cap()
{
    if (m_hCaptureReadyEvent)
    {
        CloseHandle(m_hCaptureReadyEvent);
        m_hCaptureReadyEvent = NULL;
    }

    if (m_hCaptureStopEvent)
    {
        CloseHandle(m_hCaptureStopEvent);
        m_hCaptureStopEvent = NULL;
    }
    ::DeleteCriticalSection(&m_sectionDataCb);
}

bool lc_wasa_audio_cap::Start()
{

    if (!Init())
    {
        return false;
    }

    HRESULT res = m_pAudioClient->Start();
    if (FAILED(res))
    {
        UnInit();
        return false;
    }
    return true;
}

void lc_wasa_audio_cap::Stop()
{
    SetEvent(m_hCaptureStopEvent);
    m_pAudioClient->Stop();
    bInit_ = true;
    //file.Close();
    UnInit();
}

void lc_wasa_audio_cap::SetDev(int nIndex)
{
    m_nDevIndex = nIndex;
}

void lc_wasa_audio_cap::SetCallBack(AudioDataCallBack pcb)
{
    m_pcb = pcb;
}

bool lc_wasa_audio_cap::Init()
{
    CoInitialize(NULL);
    CComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT res;
    res = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL);
    if (FAILED(res))
        return false;

   

    EDataFlow dataFlow;
    if (m_devtype == Dev_Microphone)
    {
        dataFlow = eCapture;
        m_ainfo.src = src_micphone;
    }
    else
    {
        dataFlow = eRender;
        m_ainfo.src = src_player;
    }
    //获取设备,没有制定或是获取不到制定设备则使用默认设备
    bool bNeedGetDef = true;
    if (m_nDevIndex != -1)
    {
        CComPtr<IMMDeviceCollection> pDevCollection;
        res = enumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pDevCollection);
        if (SUCCEEDED(res))
        {
            res = pDevCollection->Item(m_nDevIndex, &m_pDev);
            if (SUCCEEDED(res))
            {
                bNeedGetDef = false;
            }
        }
    }

    if (bNeedGetDef)
    {
        res = enumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &m_pDev);
        if (FAILED(res))
        {
            CleanUp();
            return false;
        }
    }
    
    res = m_pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_pAudioClient);

    if (FAILED(res))
    {
        CleanUp();
        return false;
    }

    res = m_pAudioClient->GetMixFormat(&m_pWfex);
    if (FAILED(res))
    {
        CleanUp();
        return false;
    }

    AdjustFormatTo16Bits(m_pWfex);
    m_ainfo.bytesPerSample = m_pWfex->wBitsPerSample / 8;
    m_ainfo.channels = m_pWfex->nChannels;
    m_ainfo.samplesPerSec = m_pWfex->nSamplesPerSec;
    m_ainfo.frameSize = (m_pWfex->wBitsPerSample / 8) * m_pWfex->nChannels;

    DWORD streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
    if (m_devtype == Dev_Player)
        streamFlags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

    res = m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags,
        REFERENCE_TIME_VAL, 0, m_pWfex, NULL);


    if (FAILED(res))
    {
        CleanUp();
        return false;
    }

    if (m_devtype == Dev_Player)
    {
        InitRender();
    }

    res = m_pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_pCaptureClient);
    if (FAILED(res))
    {
        CleanUp();
        return false;
    }

    res = m_pAudioClient->SetEventHandle(m_hCaptureReadyEvent);
    if (FAILED(res))
    {
        CleanUp();
        return false;
    }

    bInit_ = true;
    return true;
}

void lc_wasa_audio_cap::UnInit()
{
    CoUninitialize();
    CleanUp();
}

void lc_wasa_audio_cap::CleanUp()
{
    m_pDev.Release();
    m_pAudioClient.Release();
    m_pCaptureClient.Release();
    m_pRenderClient.Release();
}

bool lc_wasa_audio_cap::CaptureData(BYTE** ppData, UINT* nSamples, AUDIO_INFO& audioinfo)
{
    HRESULT res;
    UINT64  pos, ts;
    DWORD   flags;
    LPBYTE  buffer;
    UINT32  frames;
    UINT captureSize = 0;
    res = m_pCaptureClient->GetNextPacketSize(&captureSize);
    if (FAILED(res))
        return false;

    if (!captureSize)
        return false;

    res = m_pCaptureClient->GetBuffer(&buffer, &frames, &flags, &pos, &ts);

    if (FAILED(res))
        return false;
    if (frames)
    {
        *ppData = (BYTE*)malloc(frames * m_ainfo.frameSize);
        memcpy(*ppData, buffer, frames * m_ainfo.frameSize);
        *nSamples = frames * m_ainfo.frameSize;
    }
    m_pCaptureClient->ReleaseBuffer(frames);
    audioinfo = m_ainfo;
    return true;
}

void lc_wasa_audio_cap::Captrue(LPSTR file)
{
    //开线程处理
    if (file)
    {
        filedump_ = new lc_wavefile();
        filedump_->Create(file, m_pWfex);
    }
    CloseHandle((HANDLE)_beginthreadex(NULL, 0, &lc_wasa_audio_cap::CaptureThread, (void*)this, 0, NULL));
}

unsigned int WINAPI lc_wasa_audio_cap::CaptureThread(void* param)
{
    lc_wasa_audio_cap* pThis = (lc_wasa_audio_cap*)param;
    if (pThis)
    {
        pThis->CaptureLoopProc();
    }
    return 0;
}

void lc_wasa_audio_cap::CaptureLoopProc()
{
    HANDLE waitArray[2] = { m_hCaptureStopEvent, m_hCaptureReadyEvent };
    DWORD dwDuration = m_devtype == Dev_Microphone ? INFINITE : 10;
    while (true)
    {   
        DWORD result = WaitForMultipleObjects(2, waitArray, FALSE, dwDuration); 
        if (result == WAIT_OBJECT_0)
        {
            break;
        }
        else if (result == WAIT_OBJECT_0 + 1 || result == WAIT_TIMEOUT)
        {
            BYTE* p = NULL;
            UINT32 n = 0;
            AUDIO_INFO info;
           
            CaptureData(&p, &n, info);
            if (p)
            {
                if (filedump_)
                {
                    filedump_->WriteData(p, n);
                }

				//添加到编码线程
				PDT pdt;
				pdt.buffersize = n;
				pdt.timeTicket=GetTickCount();
				pdt.pbuffer = p;
				lc_faac_encoder::get().AddEncoderData(pdt);

                //回调用数据
                if (m_pcb)
                {
                    m_pcb(p, n, info);
                }
				//使用完成以后释放
                //free(p);
            }
            
        }  
    }
    if (filedump_)
    {
        filedump_->Close();
        delete filedump_;
        filedump_ = NULL;
    }
}

WAVEFORMATEX* lc_wasa_audio_cap::getWavEformatex()
{
	return m_pWfex;
}

bool lc_wasa_audio_cap::InitRender()
{
    WAVEFORMATEX*              wfex;
    HRESULT                    res;
    LPBYTE                     buffer;
    UINT32                     frames;
    CComPtr<IAudioClient>      client;


    res = m_pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
        nullptr, (void**)&client);
    if (FAILED(res))
    {
        return false;
    }

    res = client->GetMixFormat(&wfex);
    if (FAILED(res))
    {
        return false;
    }

    res = client->Initialize(
        AUDCLNT_SHAREMODE_SHARED, 0,
        REFERENCE_TIME_VAL, 0, wfex, nullptr);
    if (FAILED(res))
    {
        return false;
    }

    res = client->GetBufferSize(&frames);
    if (FAILED(res))
    {
        return false;
    }

    res = client->GetService(__uuidof(IAudioRenderClient),
        (void**)&m_pRenderClient);
    if (FAILED(res))
    {
        return false;
    }

    res = m_pRenderClient->GetBuffer(frames, &buffer);
    if (FAILED(res))
    {
        return false;
    }
    memset(buffer, 0, frames*wfex->nBlockAlign);
    m_pRenderClient->ReleaseBuffer(frames, 0);
    return true;
}
