#pragma once

#include "lc_device.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <propsys.h>
#include <vector>
#include <atlbase.h>
#include "lc_wasa_audio_cap.h"
#include "lc_wavefile.h"

/*
Windows wasapi获取声音,必须在windows vista以上使用
*/



typedef void(*AudioDataCallBack)(BYTE*, UINT, AUDIO_INFO);

class lc_wasa_audio_cap
{
public:
    lc_wasa_audio_cap(DevType dev);
    ~lc_wasa_audio_cap();
public:
    bool Start();
    void Stop();
    void SetDev(int nIndex);
    void SetCallBack(AudioDataCallBack pcb);
    bool CaptureData(BYTE** ppData, UINT* nSamples, AUDIO_INFO& audioinfo);
    void Captrue(LPSTR file);
    void CaptureLoopProc();
    WAVEFORMATEX* getWavEformatex();
    CRITICAL_SECTION m_sectionDataCb;
protected:
    bool Init();
    void UnInit();
    void CleanUp(); 
public:
    HANDLE m_hCaptureStopEvent;
    HANDLE m_hCaptureReadyEvent;
private:
    static unsigned int WINAPI CaptureThread(void* param);
    bool InitRender();
private:
    AudioDataCallBack m_pcb;
    bool bInit_;
    CComPtr<IMMDevice> m_pDev;
    CComPtr<IAudioClient> m_pAudioClient;
    CComPtr<IAudioCaptureClient> m_pCaptureClient;
    CComPtr<IAudioRenderClient> m_pRenderClient;
    lc_wavefile* filedump_;
    
    WAVEFORMATEX* m_pWfex;

    DevType m_devtype;
    AUDIO_INFO m_ainfo;
    int m_nDevIndex;
};