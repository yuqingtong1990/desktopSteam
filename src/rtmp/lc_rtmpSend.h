#pragma once
#include <stdint.h>
#include <WinSock2.h>

extern "C"
{
#include "rtmp.h"
#include "rtmp_sys.h"
#include "amf.h"
}

typedef struct _RTMPMetadata
{
	// video, must be h264 type
	unsigned int	nWidth;
	unsigned int	nHeight;
	unsigned int	nFrameRate;		// fps
	unsigned int	nVideoDataRate;	// bps
	unsigned int	nSpsLen;
	unsigned char	Sps[1024*1024];
	unsigned int	nPpsLen;
	unsigned char	Pps[1024*1024];

	// audio, must be aac type
	bool	        bHasAudio;
	unsigned int	nAudioSampleRate;
	unsigned int	nAudioSampleSize;
	unsigned int	nAudioChannels;
	char		    pAudioSpecCfg;
	unsigned int	nAudioSpecCfgLen;

} RTMPMetadata,*LPRTMPMetadata;

class lc_rtmpsend
{
public:
	lc_rtmpsend();
	~lc_rtmpsend();
public:
	void Init();
	bool Connect(const char* url);
	void Close();
	int  SendVideoSpsPps(unsigned char *pps,int pps_len,unsigned char * sps,int sps_len);
	bool SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp);
	bool SendMetadata(LPRTMPMetadata lpMetaData);
	int  SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);
	bool SendAACPacket(unsigned char* data,unsigned int size,unsigned int nTimeStamp);
public:
	static lc_rtmpsend& get(); 
	void Start();
	void Stop();
	void SendLoopProc();
	static unsigned int WINAPI SendThread(void* param);
	
private:
	bool bHaveSendSps_;
	HANDLE hEventStop;
	RTMPMetadata metaData; 
	RTMP* m_pRtmp;
	int video_init;
};