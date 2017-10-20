#pragma once
#include <stdint.h>

typedef struct lc_yuv_convert
{
    int width;
    int height;
    uint8_t* yBuffer;
    uint8_t* uBuffer;
    uint8_t* vBuffer;

    lc_yuv_convert();
    ~lc_yuv_convert();
    bool RGB2YUV(uint8_t* rgbData, int32_t w, int32_t h);
    void clear();
    bool Writei420Yuv(FILE* f);
}YUV;

