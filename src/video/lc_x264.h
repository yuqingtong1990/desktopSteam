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
    lc_x264_encoder(int i_width, int i_height, int profile = profile_baseline, int csp = X264_CSP_I420);
    ~lc_x264_encoder();
public:
    //本地测试函数
    void GetHeader(void** ppheader, int* pisize);
    bool Encode(std::vector<YUV> framelst, void** frame, int* fsize);
public:
    MemoryStream header_;
private:
    x264_param_t paramData;
    x264_t* x264;
    x264_picture_t* pPic_out;
    x264_picture_t* pPic_in;
};

