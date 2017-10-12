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
    long m_nSampleRate;      // ������
    int m_nChannels;         // ������
    int m_nBits;             // ������λ��
    unsigned long m_nInputSamples;	    //�������������
    unsigned long m_nMaxOutputBytes;	//�������ֽ�

    faacEncHandle m_hfaac;
    faacEncConfigurationPtr m_pfaacconf;

    unsigned char* m_pfaacbuffer;
    unsigned char*m_pfaacinfobuffer;
    unsigned long m_nfaacinfosize;
};
