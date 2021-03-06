#include "stdafx.h"
#include <atlbase.h>
#include "lc_bitmap_source.h"
#include "lc_x264.h"

lc_bitmap_destop::lc_bitmap_destop(int startX /*= 0*/, int startY /*= 0*/, int iWidth /*= 0*/, int iHeight /*= 0*/)
    :src_dc_(NULL)
    ,dst_dc_(NULL)
    ,hbmp_(NULL)
    ,width_(iWidth)
    ,height_(iHeight)
    ,bmp_buffer_(NULL)
	,m_pcb(NULL)
{
    ptStart.x = startX;
    ptStart.y = startY;

    if (iWidth == 0 || iWidth == 0)
    {
        width_ = GetSystemMetrics(SM_CXSCREEN);
        height_ = GetSystemMetrics(SM_CYSCREEN);
    }
	hEventStop = CreateEvent(NULL,FALSE,FALSE,NULL);
}

lc_bitmap_destop::~lc_bitmap_destop()
{
    if (src_dc_)
    {
        ReleaseDC(NULL, src_dc_);
        src_dc_ = NULL;
    }
    if (dst_dc_)
    {
        DeleteDC(dst_dc_);
        dst_dc_ = NULL;
    }
    if (hbmp_)
    {
        DeleteObject(hbmp_);
        hbmp_ = NULL;
    }
}

void lc_bitmap_destop::Start()
{
	Create();
	CloseHandle((HANDLE)_beginthreadex(NULL, 0, &lc_bitmap_destop::CaptureThread, (void*)this, 0, NULL));
}


unsigned int WINAPI lc_bitmap_destop::CaptureThread(void* param)
{
	lc_bitmap_destop* pThis = (lc_bitmap_destop*)param;
	if (pThis)
	{
		pThis->CaptureLoopProc();
	}
	return 0;
}


void lc_bitmap_destop::Stop()
{
	SetEvent(hEventStop);
}

void lc_bitmap_destop::Create()
{
    src_dc_ = GetDC(NULL);
    dst_dc_ = CreateCompatibleDC(src_dc_);

    BITMAPINFO bmi_;
    bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi_.bmiHeader.biWidth = width_;
    bmi_.bmiHeader.biHeight = -height_;
    bmi_.bmiHeader.biPlanes = 1;
    bmi_.bmiHeader.biBitCount = 24;
    bmi_.bmiHeader.biCompression = BI_RGB;
    bmi_.bmiHeader.biSizeImage = 0;
    bmi_.bmiHeader.biXPelsPerMeter = 0;
    bmi_.bmiHeader.biYPelsPerMeter = 0;
    bmi_.bmiHeader.biClrUsed = 0;
    bmi_.bmiHeader.biClrImportant = 0;

    hbmp_ = CreateDIBSection(dst_dc_, &bmi_, DIB_RGB_COLORS, &bmp_buffer_, NULL, 0);
    int a = GetLastError();
    SelectObject(dst_dc_, hbmp_);
    refresh();
}

void lc_bitmap_destop::CaptureLoopProc()
{
	HANDLE waitArray[1] = { hEventStop};
	while (true)
	{   
		DWORD result = WaitForMultipleObjects(1, waitArray, FALSE, 1000); 
		if (result == WAIT_OBJECT_0)
		{
			break;
		}
		else if (result == WAIT_TIMEOUT)
		{
			refresh();
			YUV yuv;
			yuv.RGB2YUV((uint8_t*)rgbData(),width_,height_);
			lc_x264_encoder::get().AddtoEncodeLst(yuv);
			if (m_pcb)
			{
				m_pcb(yuv);
			}	
		}
	}
}

void lc_bitmap_destop::refresh()
{
    if (dst_dc_ == NULL || src_dc_ == NULL)
        return;

    ::BitBlt(dst_dc_, 0, 0, width_, height_, src_dc_, ptStart.x, ptStart.y, SRCCOPY);
}

void* lc_bitmap_destop::rgbData()
{
    return bmp_buffer_;
}

HBITMAP lc_bitmap_destop::getHBitmap()
{
    return hbmp_;
}

void lc_bitmap_destop::SetCallBack(videoDateCallBack pcb)
{
	m_pcb = pcb;
}

// int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
// {
//     UINT  num = 0;// number of image encoders      
//     UINT  size = 0;// size of the image encoder array in bytes      
// 
//     ImageCodecInfo* pImageCodecInfo = NULL;
//     GetImageEncodersSize(&num, &size);
//     if (size == 0)
//         return -1;  // Failure      
// 
//     pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
//     if (pImageCodecInfo == NULL)
//         return -1;  // Failure      
// 
//     GetImageEncoders(num, size, pImageCodecInfo);
// 
//     for (UINT j = 0; j < num; ++j)
//     {
//         if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
//         {
//             *pClsid = pImageCodecInfo[j].Clsid;
//             free(pImageCodecInfo);
//             return j;  // Success      
//         }
//     }
// 
//     free(pImageCodecInfo);
//     return -1;  // Failure  
// }
// 
// bool SaveBitmapAsfmt(HBITMAP hBitmap, WCHAR* fmt, LPCTSTR lpFileName)
// {
//     if (hBitmap == NULL)
//         return false;
// 
//     BITMAP bm;
//     GetObject(hBitmap, sizeof(BITMAP), &bm);
//     WORD BitsPerPixel = bm.bmBitsPixel;
// 
//     Bitmap* bitmap = Bitmap::FromHBITMAP(hBitmap, NULL);
//     EncoderParameters encoderParameters;
//     ULONG compression;
//     CLSID clsid;
// 
//     if (BitsPerPixel == 1)
//     {
//         compression = EncoderValueCompressionCCITT4;
//     }
//     else
//     {
//         compression = EncoderValueCompressionLZW;
//     }
//     GetEncoderClsid(fmt, &clsid);
//     encoderParameters.Count = 1;
//     encoderParameters.Parameter[0].Guid = EncoderQuality;
//     encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
//     encoderParameters.Parameter[0].NumberOfValues = 1;
//     encoderParameters.Parameter[0].Value = &compression;
//     bitmap->Save(lpFileName, &clsid, &encoderParameters);
//     return true;
// }
