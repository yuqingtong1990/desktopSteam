#pragma once
#include "lc_util.h"
#include "lc_yuv.h"
/*
*autho:yuqingtong
*email：351047769@qq.com
*date:2017.08.08
*detial:X264编码类
*/
extern "C"
{
#include "x264/x264.h"
#include "x264/x264_config.h"
}

#include <vector>
#include <string>

enum x264_profiles{
    profile_baseline = 0,
    profile_main,
    profile_high,
    profile_high10,
    profile_high422,
    profile_high444,
};

class lc_x264_encoder
{
public:
    lc_x264_encoder();
    ~lc_x264_encoder();
public:
	void Init(int i_width, int i_height, int profile = profile_baseline, int csp = X264_CSP_I420);
	void Start();
	void Stop();
    //本地测试函数
	PDT getPdt();
	int64_t getFirstFrameTime();
    void GetHeader(void** ppheader, int* pisize);
    bool Encodelst(std::vector<YUV> framelst, void** frames, int* fsize);
	bool EncodeOne(const YUV& yuv, PDT& pdt);
	void AddtoEncodeLst(const YUV& yuv);
	void EncodeProcess();	
    MemoryStream header_;
	static lc_x264_encoder& get();
private:
	static unsigned int WINAPI EncodeThread(void* param);
private:
	std::vector<PDT> m_vecheader;
	std::vector<YUV> m_vecWaitEncode;
	std::vector<PDT> m_vecHaveEncode;
	CRITICAL_SECTION m_HaveSection;
	CRITICAL_SECTION m_PreSection;
    x264_param_t paramData; 
    x264_t* x264;
    x264_picture_t* pPic_out;
    x264_picture_t* pPic_in;
	HANDLE m_hEventStop;
	HANDLE m_hEventReady;
	
};

