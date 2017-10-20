#include "stdafx.h"
#include <stdint.h>
#include <stdio.h>
#include "lc_util.h"
#include "lc_yuv.h"

// bool ConvertRGB2YUV(int32_t w, int32_t h, uint8_t* rgbData, uint8_t* y, uint8_t* u, uint8_t*v)
// {
//     float RGBYUV02990[256], RGBYUV05870[256], RGBYUV01140[256];
//     float RGBYUV01684[256], RGBYUV03316[256];
//     float RGBYUV04187[256], RGBYUV00813[256];
// 
//     for (int i = 0; i < 256; i++)
//     {
//         RGBYUV02990[i] = (float)0.2990 * i;
//         RGBYUV05870[i] = (float)0.5870 * i;
//         RGBYUV01140[i] = (float)0.1140 * i;
//         RGBYUV01684[i] = (float)0.1684 * i;
//         RGBYUV03316[i] = (float)0.3316 * i;
//         RGBYUV04187[i] = (float)0.4187 * i;
//         RGBYUV00813[i] = (float)0.0813 * i;
//     }
// 
//     unsigned char *ytemp = NULL;
//     unsigned char *utemp = NULL;
//     unsigned char *vtemp = NULL;
//     utemp = (unsigned char *)malloc(w*h);
//     vtemp = (unsigned char *)malloc(w*h);
// 
//     int i, nr, ng, nb, nSize;
//     //对每个像素进行 rgb -> yuv的转换
//     for (i = 0, nSize = 0; nSize < w*h * 3; nSize += 3)
//     {
//         nb = rgbData[nSize];
//         ng = rgbData[nSize + 1];
//         nr = rgbData[nSize + 2];
//         y[i] = (unsigned char)(RGBYUV02990[nr] + RGBYUV05870[ng] + RGBYUV01140[nb]);
//         utemp[i] = (unsigned char)(-RGBYUV01684[nr] - RGBYUV03316[ng] + nb / 2 + 128);
//         vtemp[i] = (unsigned char)(nr / 2 - RGBYUV04187[ng] - RGBYUV00813[nb] + 128);
//         i++;
//     }
//     //对u信号及v信号进行采样
//     int k = 0;
//     for (i = 0; i < h; i += 2)
//         for (int j = 0; j < w; j += 2)
//         {
//             u[k] = (utemp[i*w + j] + utemp[(i + 1)*w + j] + utemp[i*w + j + 1] + utemp[(i + 1)*w + j + 1]) / 4;
//             v[k] = (vtemp[i*w + j] + vtemp[(i + 1)*w + j] + vtemp[i*w + j + 1] + vtemp[(i + 1)*w + j + 1]) / 4;
//             k++;
//         }
//     //对y、u、v 信号进行抗噪处理
//     for (i = 0; i < w*h; i++)
//     {
//         if (y[i] < 16)
//             y[i] = 16;
//         if (y[i] > 235)
//             y[i] = 235;
//     }
//     for (i = 0; i < h*w / 4; i++)
//     {
//         if (u[i] < 16)
//             u[i] = 16;
//         if (v[i] < 16)
//             v[i] = 16;
//         if (u[i] > 240)
//             u[i] = 240;
//         if (v[i] > 240)
//             v[i] = 240;
//     }
//     if (utemp)
//         free(utemp);
//     if (vtemp)
//         free(vtemp);
//     return true;
// }
// 
bool WriteYuvAsi420(FILE* f, int32_t w, int32_t h, uint8_t * y, uint8_t * u, uint8_t *v)
{
    unsigned int size = w*h;

    if (fwrite(y, 1, size, f) != size)
        return false;

    if (fwrite(v, 1, size / 4, f) != size / 4)
        return false;

    if (fwrite(u, 1, size / 4, f) != size / 4)
        return false;
    return true;
}
// 
// void ConvertRGB24YUVI420(int32_t w, int32_t h, uint8_t *bmp, uint8_t *yuv)
// {
//     int RGB2YUV_YR[256], RGB2YUV_YG[256], RGB2YUV_YB[256];
//     int RGB2YUV_UR[256], RGB2YUV_UG[256], RGB2YUV_UBVR[256];
//     int RGB2YUV_VG[256], RGB2YUV_VB[256];
// 
//     int32_t n;
//     for (n = 0; n < 256; n++) RGB2YUV_YR[n] = (float)65.481 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_YG[n] = (float)128.553 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_YB[n] = (float)24.966 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_UR[n] = (float)37.797 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_UG[n] = (float)74.203 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_VG[n] = (float)93.786 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_VB[n] = (float)18.214 * (n << 8);
//     for (n = 0; n < 256; n++) RGB2YUV_UBVR[n] = (float)112 * (n << 8);
// 
// 
//     uint8_t *u, *v, *y, *uu, *vv;
//     uint8_t *pu1, *pu2, *pu3, *pu4;
//     uint8_t *pv1, *pv2, *pv3, *pv4;
//     uint8_t *r, *g, *b;
//     int i, j;
// 
//     uu = (uint8_t*)malloc(w*h * 2);
//     vv = (uint8_t*)malloc(w*h * 2);
// 
//     if (uu == NULL || vv == NULL)
//         return;
// 
//     y = yuv;
//     u = uu;
//     v = vv;
//     // Get r,g,b pointers from bmp image data....
//     r = bmp;
//     g = bmp + 1;
//     b = bmp + 2;
//     //Get YUV values for rgb values...
//     for (i = 0; i < h; i++)
//     {
//         for (j = 0; j < w; j++)
//         {
//             *y++ = (RGB2YUV_YR[*r] + RGB2YUV_YG[*g] + RGB2YUV_YB[*b] + 1048576) >> 16;
//             *u++ = (-RGB2YUV_UR[*r] - RGB2YUV_UG[*g] + RGB2YUV_UBVR[*b] + 8388608) >> 16;
//             *v++ = (RGB2YUV_UBVR[*r] - RGB2YUV_VG[*g] - RGB2YUV_VB[*b] + 8388608) >> 16;
//             r += 3;
//             g += 3;
//             b += 3;
//         }
//     }
// 
//     // Get the right pointers...
//     //I420 yvu YV12 yuv
//     v = yuv + w*h;
//     u = v + (w*h) / 4;
// 
//     // For U
//     pu1 = uu;
//     pu2 = pu1 + 1;
//     pu3 = pu1 + w;
//     pu4 = pu3 + 1;
// 
//     // For V
//     pv1 = vv;
//     pv2 = pv1 + 1;
//     pv3 = pv1 + w;
//     pv4 = pv3 + 1;
// 
//     // Do sampling....
//     for (i = 0; i < h; i += 2)
//     {
//         for (j = 0; j < w; j += 2)
//         {
//             *u++ = (*pu1 + *pu2 + *pu3 + *pu4) >> 2;
//             *v++ = (*pv1 + *pv2 + *pv3 + *pv4) >> 2;
//             pu1 += 2;
//             pu2 += 2;
//             pu3 += 2;
//             pu4 += 2;
// 
//             pv1 += 2;
//             pv2 += 2;
//             pv3 += 2;
//             pv4 += 2;
//         }
// 
//         pu1 += w;
//         pu2 += w;
//         pu3 += w;
//         pu4 += w;
// 
//         pv1 += w;
//         pv2 += w;
//         pv3 += w;
//         pv4 += w;
// 
//     }
// 
//     free(uu);
//     free(vv);
// }
// 
// bool WriteYuv(FILE* f, int32_t w, int32_t h, uint8_t * yuv)
// {
//     unsigned int size = w*h * 3 / 2;
// 
//     if (fwrite(yuv, 1, size, f) != size)
//         return false;
//     return true;
// }

lc_yuv_convert::lc_yuv_convert()
    :width(0)
    ,height(0)
    ,yBuffer(NULL)
    ,uBuffer(NULL)
    ,vBuffer(NULL)
{

}

lc_yuv_convert::~lc_yuv_convert()
{
	clear();
}

bool lc_yuv_convert::RGB2YUV(uint8_t* rgbData, int32_t w, int32_t h)
{
    width = w;
    height = h;

    yBuffer = (uint8_t *)malloc(w*h);
    uBuffer = (uint8_t *)malloc(w*h * 1 / 4);
    vBuffer = (uint8_t *)malloc(w*h * 1 / 4);
    return ConvertRGB2YUV(w, h, rgbData, yBuffer, uBuffer, vBuffer);   
}

void lc_yuv_convert::clear()
{
    if (yBuffer){
        free(yBuffer);
        yBuffer = NULL;
    }
    if (uBuffer){
        free(uBuffer);
        uBuffer = NULL;
    }
    if (vBuffer){
        free(vBuffer);
        vBuffer = NULL;
    }
}

bool lc_yuv_convert::Writei420Yuv(FILE* f)
{
    return WriteYuvAsi420(f, width, height, yBuffer, uBuffer, vBuffer);
}
