#pragma once
#include "lc_device.h"
#include "lc_wavefile.h"
#include "lc_audio_mix.h"
#include "lc_faac.h"
/*
*pcm混音
*/
void VolumeMix(BYTE* buf, int size, BYTE* buf1, int size1);
void MixAudio(float *bufferDest, float *bufferSrc, UINT totalFloats, bool bForceMono);
//转换为float类型
void converttofloat(void* buffer, int nsz, float** fbuffer, UINT* fsz, int wBitsPerSample, int frames, int channels);

//从双声道提取左声道
unsigned char * get_oneChannel_left_from_doubleChannel(unsigned char * pDoubleChannelBuf, int nLen, int nPerSampleBytesPerChannle);


class lc_audio_mixing
{
public:
    lc_audio_mixing();
    ~lc_audio_mixing();
    static lc_audio_mixing& get();
    void SetFile(lc_wavefile* file = NULL);
    void SetFile(FILE* file = NULL);
    static void FillAudioData(BYTE* pbuffer, UINT uSize, AUDIO_INFO info);
    void FillData(BYTE* pbuffer, UINT uSize, AUDIO_INFO info);
private:
    lc_faac_encoder* faccEncoder;
    lc_wavefile* file_;
    FILE* fileacc;
    static lc_audio_mixing instace;
    CRITICAL_SECTION m_DataSection;
    BYTE* pBuffer_;
    UINT uSize_;
};