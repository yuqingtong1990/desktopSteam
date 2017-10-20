#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include "lc_util.h"

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
	void Start();
	void Stop();
	unsigned char* Encoder(unsigned char* indata, int inlen, int &outlen);
	PDT getPdt();
	void Eraseit();
	int64_t getFirstFrameTime();
	void EncoderProcess();
	bool IsWorking(void);
	void AddEncoderData(const PDT& pdt);
	static lc_faac_encoder& get();
private: 
	static unsigned int WINAPI EncodeThread(void* param);
	bool isWork_;
	FILE* m_pFile;
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
	HANDLE hEventStop;
	HANDLE hEventReady;
	CRITICAL_SECTION m_PreSection;
	CRITICAL_SECTION m_HaveSection;

	std::vector<PDT> m_vecWaitEncode;//�������б�����
	std::vector<PDT> m_vecHaveEncode;//����ɱ�������
};
