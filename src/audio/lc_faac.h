#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern "C"
{
#include "faac.h"
#include "faaccfg.h"
}

class lc_faac_encoder  
{
public:
	lc_faac_encoder();
	virtual ~lc_faac_encoder();
	int Clean();
	int Init(int nSampleRate, int nChannels);
	unsigned char* Encoder(unsigned char* indata, int inlen, int &outlen);
	bool IsWorking(void);
	bool GetInfo(unsigned char* data,int& len);
private:
    long m_nSampleRate;      // 采样率
    int m_nChannels;         // 声道数
    int m_nBits;             // 单样本位数
    unsigned long m_nInputSamples;	    //最大输入样本数
    unsigned long m_nMaxOutputBytes;	//最大输出字节

    faacEncHandle m_hfaac;
    faacEncConfigurationPtr m_pfaacconf;

    unsigned char* m_pfaacbuffer;
    unsigned char*m_pfaacinfobuffer;
    unsigned long m_nfaacinfosize;
};
