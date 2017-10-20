#pragma once

#include <gdiplus.h>  
using namespace Gdiplus;
#pragma comment(lib,"gdiplus")
#include "lc_yuv.h"


typedef void(*videoDateCallBack)(const YUV& yuv);

class lc_bitmap_destop
{
public:
    lc_bitmap_destop(int startX = 0, int startY = 0, int iWidth = 0, int iHeight = 0);
    ~lc_bitmap_destop();
public:
	void Start();
	void Stop();
    void Create();
	void CaptureLoopProc();
    void refresh();
    void* rgbData();
    int width(){ return width_; }
    int height(){ return height_; }
    HBITMAP getHBitmap();
	void SetCallBack(videoDateCallBack pcb);
private:
    static unsigned int WINAPI CaptureThread(void* param);
private:
    HDC src_dc_;
    HDC dst_dc_;
    HBITMAP hbmp_;
    void* bmp_buffer_;
    int width_;
    int height_;
    POINT ptStart;
	HANDLE hEventStop;
	videoDateCallBack m_pcb;
};

bool SaveBitmapAsfmt(HBITMAP hBitmap, WCHAR* fmt, LPCTSTR lpFileName);