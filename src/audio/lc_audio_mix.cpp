#include "stdafx.h"
#include "lc_util.h"
#include "lc_audio_mix.h"

typedef unsigned long long  UPARAM;


union TripleToLong
{
    LONG val;
    struct
    {
        WORD wVal;
        BYTE tripleVal;
        BYTE lastByte;
    };
};

void converttofloat(void* buffer, int nsz, float** fbuffer, UINT* fsz, int wBitsPerSample, int frames, int channels)
{
    UINT totalSamples = frames*channels;
    *fsz = totalSamples;
    float* f = new (std::nothrow) float[totalSamples];
    if (wBitsPerSample == 8)
    {
        float *tempConvert = f;
        char *tempSByte = (char*)buffer;
        while (totalSamples--)
        {
            *(tempConvert++) = float(*(tempSByte++)) / 127.0f;
        }
    }
    else if (wBitsPerSample == 16)
    {
        float *tempConvert = f;
        short *tempShort = (short*)buffer;

        while (totalSamples--)
        {
            *(tempConvert++) = float(*(tempShort++)) / 32767.0f;
        }
    }
    else if (wBitsPerSample == 24)
    {
        float *tempConvert = f;
        BYTE *tempTriple = (BYTE*)buffer;
        TripleToLong valOut;

        while (totalSamples--)
        {
            TripleToLong &valIn = (TripleToLong&)tempTriple;

            valOut.wVal = valIn.wVal;
            valOut.tripleVal = valIn.tripleVal;
            if (valOut.tripleVal > 0x7F)
                valOut.lastByte = 0xFF;

            *(tempConvert++) = float(double(valOut.val) / 8388607.0);
            tempTriple += 3;
        }
    }
    else if (wBitsPerSample == 32)
    {
        float *tempConvert = f;
        long *tempShort = (long*)buffer;

        while (totalSamples--)
        {
            *(tempConvert++) = float(double(*(tempShort++)) / 2147483647.0);
        }
    }
    *fbuffer = f;
}

unsigned char * get_oneChannel_left_from_doubleChannel(unsigned char * pDoubleChannelBuf, int nLen, int nPerSampleBytesPerChannle)
{
    int nOneChannelLen = nLen / 2;
    unsigned char * pOneChannelBuf = new unsigned char[nOneChannelLen];
    for (int i = 0; i < nOneChannelLen / 2; i++)
    {
        memcpy((uint16_t*)pOneChannelBuf + i, ((uint32_t *)(pDoubleChannelBuf)) + i, nPerSampleBytesPerChannle);
    }
    return pOneChannelBuf;
}

//RAII²Ù×÷·â×°Àà


// ÒôÆµ»ìÒô
void VolumeMix(BYTE* buf, int size, BYTE* buf1, int size1)
{
    if (buf == NULL || buf1 == NULL)
    {
        return;
    }

    int  size_min = size > size1 ? size1 : size1;

    for (int i = 0; i < size_min;)
    {
        signed long minData = -0x8000;
        signed long maxData = 0x7FFF;

        signed short wData = buf[i + 1];
        wData = MAKEWORD(buf[i], buf[i + 1]);
        signed long dwData = wData;

        signed short wData1 = buf1[i + 1];
        wData1 = MAKEWORD(buf1[i], buf1[i + 1]);
        signed long dwData1 = wData1;



        dwData = dwData + dwData1;
        if (dwData < -0x8000)
        {
            dwData = -0x8000;
        }
        else if (dwData > 0x7FFF)
        {
            dwData = 0x7FFF;
        }

        wData = LOWORD(dwData);
        buf[i] = LOBYTE(wData);
        buf[i + 1] = HIBYTE(wData);
        i += 2;
    }
}

void MixAudio(float *bufferDest, float *bufferSrc, UINT totalFloats, bool bForceMono)
{
    UINT floatsLeft = totalFloats;
    float *destTemp = bufferDest;
    float *srcTemp = bufferSrc;

    if ((UPARAM(destTemp) & 0xF) == 0 && (UPARAM(srcTemp) & 0xF) == 0)
    {
        UINT alignedFloats = floatsLeft & 0xFFFFFFFC;

        if (bForceMono)
        {
            __m128 halfVal = _mm_set_ps1(0.5f);
            for (UINT i = 0; i < alignedFloats; i += 4)
            {
                float *micInput = srcTemp + i;
                __m128 val = _mm_load_ps(micInput);
                __m128 shufVal = _mm_shuffle_ps(val, val, _MM_SHUFFLE(2, 3, 0, 1));

                _mm_store_ps(micInput, _mm_mul_ps(_mm_add_ps(val, shufVal), halfVal));
            }
        }

        __m128 maxVal = _mm_set_ps1(1.0f);
        __m128 minVal = _mm_set_ps1(-1.0f);

        for (UINT i = 0; i < alignedFloats; i += 4)
        {
            float *pos = destTemp + i;

            __m128 mix;
            mix = _mm_add_ps(_mm_load_ps(pos), _mm_load_ps(srcTemp + i));
            mix = _mm_min_ps(mix, maxVal);
            mix = _mm_max_ps(mix, minVal);

            _mm_store_ps(pos, mix);
        }

        floatsLeft &= 0x3;
        destTemp += alignedFloats;
        srcTemp += alignedFloats;
    }

    if (floatsLeft)
    {
        if (bForceMono)
        {
            for (UINT i = 0; i < floatsLeft; i += 2)
            {
                srcTemp[i] += srcTemp[i + 1];
                srcTemp[i] *= 0.5f;
                srcTemp[i + 1] = srcTemp[i];
            }
        }

        for (UINT i = 0; i < floatsLeft; i++)
        {
            float val = destTemp[i] + srcTemp[i];

            if (val < -1.0f)     val = -1.0f;
            else if (val > 1.0f) val = 1.0f;

            destTemp[i] = val;
        }
    }
}

lc_audio_mixing::lc_audio_mixing()
    :pBuffer_(NULL)
    ,uSize_(0)
    ,fileacc(NULL)
{
    InitializeCriticalSection(&m_DataSection); 
    pBuffer_ = (BYTE*)malloc(10000);
    memset(pBuffer_, 0, 10000);
    
    faccEncoder = new lc_faac_encoder();
    faccEncoder->Init(44100, 1);
}

lc_audio_mixing::~lc_audio_mixing()
{
    delete faccEncoder;
    DeleteCriticalSection(&m_DataSection);
}

lc_audio_mixing& lc_audio_mixing::get()
{
    static lc_audio_mixing s_instance;
    return s_instance;
}

void lc_audio_mixing::SetFile(lc_wavefile* file /*= NULL*/)
{
    file_ = file;
}

void lc_audio_mixing::SetFile(FILE* file /*= NULL*/)
{
    fileacc = file;
}

void lc_audio_mixing::FillAudioData(BYTE* pbuffer, UINT uSize, AUDIO_INFO info)
{
    lc_audio_mixing::get().FillData(pbuffer, uSize, info);
}

void lc_audio_mixing::FillData(BYTE* pFill, UINT uSize, AUDIO_INFO info)
{
    AutoLock cs(&m_DataSection);

    BYTE* pMix;
    UINT szMix;
    if (info.channels == 2)
    {
        pMix = get_oneChannel_left_from_doubleChannel(pFill, uSize, 2);
        szMix = uSize / 2;
    }
    else
    {
        pMix = pFill;
        szMix = uSize;
    }

    if (pFill == NULL)
    {
        return;
    }
    char ch[300] = {0};
    _snprintf_s(ch, 300, _TRUNCATE, "frame size is %d ,src is %s ,frameSize is %d\r\n", uSize, info.src == src_player ? "player" : "micphone", info.frameSize);
    OutputDebugStringA(ch);
    if (uSize_ == 0)
    {
        memcpy(pBuffer_, pMix, szMix);
        uSize_ = szMix;
    }
    else
    {
        VolumeMix(pBuffer_, uSize_, pMix, szMix);
        if (file_)
        {
            file_->WriteData(pBuffer_, uSize_);
        }

        //½øÐÐ±àÂë
        int outlen = 0;
        unsigned char* outbuf = faccEncoder->Encoder(pBuffer_, uSize_, outlen);
        size_t sz = fwrite(outbuf, 1, outlen, fileacc);
        memset(pBuffer_, 0, 10000);
        uSize_ = 0;
    }
}
