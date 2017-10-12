#pragma once

#include <gdiplus.h>  
using namespace Gdiplus;
#pragma comment(lib,"gdiplus")


class lc_bitmap_destop
{
public:
    lc_bitmap_destop(int startX = 0, int startY = 0, int iWidth = 0, int iHeight = 0);
    ~lc_bitmap_destop();
public:
    void Create();
    void refresh();
    void* rgbData();
    int width(){ return width_; }
    int height(){ return height_; }
    HBITMAP getHBitmap();
private:
    HDC src_dc_;
    HDC dst_dc_;
    HBITMAP hbmp_;
    void* bmp_buffer_;
    int width_;
    int height_;
    POINT ptStart;
};

bool SaveBitmapAsfmt(HBITMAP hBitmap, WCHAR* fmt, LPCTSTR lpFileName);