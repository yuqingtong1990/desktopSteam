#include "lc_util.h"

#include <gdiplus.h>  
using namespace Gdiplus;
#pragma comment(lib,"gdiplus")


GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR  gdiplusToken;

void InitGdiplus()
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void UnInitGdiplus()
{
    GdiplusShutdown(gdiplusToken);
}

HBITMAP hbmp_ = NULL;

void GetDestopAsBmp(void** bmp,int* iWidth, int* iHeight)
{
    HDC src_dc_;
    HDC dst_dc_;
    void* bmp_buffer_;
    src_dc_ = GetDC(NULL);
    dst_dc_ = CreateCompatibleDC(src_dc_);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    BITMAPINFO bmi_;
    bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi_.bmiHeader.biWidth = width;
    bmi_.bmiHeader.biHeight = -height;
    bmi_.bmiHeader.biPlanes = 1;
    bmi_.bmiHeader.biBitCount = 24;
    bmi_.bmiHeader.biCompression = BI_RGB;
    bmi_.bmiHeader.biSizeImage = 0;
    bmi_.bmiHeader.biXPelsPerMeter = 0;
    bmi_.bmiHeader.biYPelsPerMeter = 0;
    bmi_.bmiHeader.biClrUsed = 0;
    bmi_.bmiHeader.biClrImportant = 0;

    hbmp_ = CreateDIBSection(dst_dc_, &bmi_, DIB_RGB_COLORS, &bmp_buffer_, NULL, 0);
    if (!hbmp_)
    {
        return;
    }
    SelectObject(dst_dc_, hbmp_);
    BitBlt(dst_dc_, 0, 0,width, height, src_dc_,0, 0,SRCCOPY);
    *bmp = bmp_buffer_;
    *iWidth = width;
    *iHeight = height;

    SaveBitmapAsfmt(hbmp_, L"image/bmp", L"1.bmp");
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

}

void ClearBmp()
{
    if (hbmp_)
    {
        DeleteObject(hbmp_);
        hbmp_ = NULL;
    }
}


void ConvertRGB24YUVI420(int32_t w, int32_t h, uint8_t *bmp, uint8_t *yuv)
{
    int RGB2YUV_YR[256], RGB2YUV_YG[256], RGB2YUV_YB[256];
    int RGB2YUV_UR[256], RGB2YUV_UG[256], RGB2YUV_UBVR[256];
    int RGB2YUV_VG[256], RGB2YUV_VB[256];

    int32_t n;
    for (n = 0; n < 256; n++) RGB2YUV_YR[n] = (float)65.481 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_YG[n] = (float)128.553 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_YB[n] = (float)24.966 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_UR[n] = (float)37.797 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_UG[n] = (float)74.203 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_VG[n] = (float)93.786 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_VB[n] = (float)18.214 * (n << 8);
    for (n = 0; n < 256; n++) RGB2YUV_UBVR[n] = (float)112 * (n << 8);


    uint8_t *u, *v, *y, *uu, *vv;
    uint8_t *pu1, *pu2, *pu3, *pu4;
    uint8_t *pv1, *pv2, *pv3, *pv4;
    uint8_t *r, *g, *b;
    int i, j;

    uu = (uint8_t*)malloc(w*h*2);
    vv = (uint8_t*)malloc(w*h*2);

    if (uu == NULL || vv == NULL)
        return;

    y = yuv;
    u = uu;
    v = vv;
    // Get r,g,b pointers from bmp image data....
    r = bmp;
    g = bmp + 1;
    b = bmp + 2;
    //Get YUV values for rgb values...
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            *y++ = (RGB2YUV_YR[*r] + RGB2YUV_YG[*g] + RGB2YUV_YB[*b] + 1048576) >> 16;
            *u++ = (-RGB2YUV_UR[*r] - RGB2YUV_UG[*g] + RGB2YUV_UBVR[*b] + 8388608) >> 16;
            *v++ = (RGB2YUV_UBVR[*r] - RGB2YUV_VG[*g] - RGB2YUV_VB[*b] + 8388608) >> 16;
            r += 3;
            g += 3;
            b += 3;
        }
    }

    // Get the right pointers...
    //I420 yvu YV12 yuv
    v = yuv + w*h;
    u = v + (w*h) / 4;

    // For U
    pu1 = uu;
    pu2 = pu1 + 1;
    pu3 = pu1 + w;
    pu4 = pu3 + 1;

    // For V
    pv1 = vv;
    pv2 = pv1 + 1;
    pv3 = pv1 + w;
    pv4 = pv3 + 1;

    // Do sampling....
    for (i = 0; i < h; i += 2)
    {
        for (j = 0; j < w; j += 2)
        {
            *u++ = (*pu1 + *pu2 + *pu3 + *pu4) >> 2;
            *v++ = (*pv1 + *pv2 + *pv3 + *pv4) >> 2;
            pu1 += 2;
            pu2 += 2;
            pu3 += 2;
            pu4 += 2;

            pv1 += 2;
            pv2 += 2;
            pv3 += 2;
            pv4 += 2;
        }

        pu1 += w;
        pu2 += w;
        pu3 += w;
        pu4 += w;

        pv1 += w;
        pv2 += w;
        pv3 += w;
        pv4 += w;

    }

    free(uu);
    free(vv);
}

bool ConvertRGB2YUV(int32_t w, int32_t h, uint8_t* rgbData, uint8_t* y, uint8_t* u, uint8_t*v)
{
    float RGBYUV02990[256], RGBYUV05870[256], RGBYUV01140[256];
    float RGBYUV01684[256], RGBYUV03316[256];
    float RGBYUV04187[256], RGBYUV00813[256];

    for (int i = 0; i < 256; i++)
    {
        RGBYUV02990[i] = (float)0.2990 * i;
        RGBYUV05870[i] = (float)0.5870 * i;
        RGBYUV01140[i] = (float)0.1140 * i;
        RGBYUV01684[i] = (float)0.1684 * i;
        RGBYUV03316[i] = (float)0.3316 * i;
        RGBYUV04187[i] = (float)0.4187 * i;
        RGBYUV00813[i] = (float)0.0813 * i;
    }

    unsigned char *ytemp = NULL;
    unsigned char *utemp = NULL;
    unsigned char *vtemp = NULL;
    utemp = (unsigned char *)malloc(w*h);
    vtemp = (unsigned char *)malloc(w*h);

    int i, nr, ng, nb, nSize;
    //对每个像素进行 rgb -> yuv的转换
    for (i = 0, nSize = 0; nSize < w*h * 3; nSize += 3)
    {
        nb = rgbData[nSize];
        ng = rgbData[nSize + 1];
        nr = rgbData[nSize + 2];
        y[i] = (unsigned char)(RGBYUV02990[nr] + RGBYUV05870[ng] + RGBYUV01140[nb]);
        utemp[i] = (unsigned char)(-RGBYUV01684[nr] - RGBYUV03316[ng] + nb / 2 + 128);
        vtemp[i] = (unsigned char)(nr / 2 - RGBYUV04187[ng] - RGBYUV00813[nb] + 128);
        i++;
    }
    //对u信号及v信号进行采样
    int k = 0;
    for (i = 0; i < h; i += 2)
        for (unsigned long j = 0; j < w; j += 2)
        {
            u[k] = (utemp[i*w + j] + utemp[(i + 1)*w + j] + utemp[i*w + j + 1] + utemp[(i + 1)*w + j + 1]) / 4;
            v[k] = (vtemp[i*w + j] + vtemp[(i + 1)*w + j] + vtemp[i*w + j + 1] + vtemp[(i + 1)*w + j + 1]) / 4;
            k++;
        }
    //对y、u、v 信号进行抗噪处理
    for (i = 0; i < w*h; i++)
    {
        if (y[i] < 16)
            y[i] = 16;
        if (y[i] > 235)
            y[i] = 235;
    }
    for (i = 0; i < h*w / 4; i++)
    {
        if (u[i] < 16)
            u[i] = 16;
        if (v[i] < 16)
            v[i] = 16;
        if (u[i] > 240)
            u[i] = 240;
        if (v[i] > 240)
            v[i] = 240;
    }
    if (utemp)
        free(utemp);
    if (vtemp)
        free(vtemp);
    return true;
}

bool WriteYuv(FILE* f, int32_t w, int32_t h, uint8_t * y, uint8_t * u, uint8_t *v)
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

bool WriteYuv(FILE* f, int32_t w, int32_t h, uint8_t * yuv)
{
    unsigned int size = w*h * 3 / 2;

    if (fwrite(yuv, 1, size, f) != size)
        return false;
    return true;
}

// image/bmp
// image/jpeg
// image/gif
// image/x-emf
// image/x-wmf
// image/tiff
// image/png
// image/x-icon

int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
    UINT  num = 0;// number of image encoders      
    UINT  size = 0;// size of the image encoder array in bytes      

    ImageCodecInfo* pImageCodecInfo = NULL;
    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure      

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure      

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success      
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure  
}

bool SaveBitmapAsfmt(HBITMAP hBitmap, WCHAR* fmt, LPCTSTR lpFileName)
{
    if (hBitmap == NULL)
        return false;

    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    WORD BitsPerPixel = bm.bmBitsPixel;

    Bitmap* bitmap = Bitmap::FromHBITMAP(hBitmap, NULL);
    EncoderParameters encoderParameters;
    ULONG compression;
    CLSID clsid;

    if (BitsPerPixel == 1)
    {
        compression = EncoderValueCompressionCCITT4;
    }
    else
    {
        compression = EncoderValueCompressionLZW;
    }
    GetEncoderClsid(fmt, &clsid);
    encoderParameters.Count = 1;
    encoderParameters.Parameter[0].Guid = EncoderQuality;
    encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
    encoderParameters.Parameter[0].NumberOfValues = 1;
    encoderParameters.Parameter[0].Value = &compression;
    bitmap->Save(lpFileName, &clsid, &encoderParameters);
    return true;
}

inline void IncPtr(void **p, int i){ *p = (void*)((int)*p + i); }

bool FileExists(const std::wstring &fn)
{
    WIN32_FIND_DATAW fd;
    HANDLE hFile = FindFirstFileW(fn.c_str(), &fd);
    return (hFile != INVALID_HANDLE_VALUE);
}

bool LoadFileToBuffer(const std::wstring &file,void **buf,DWORD &bufsize)
{
    bool bSuccess = false;
    if (!FileExists(file))
        return false;
    HANDLE h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            DWORD iSize = GetFileSize(h, NULL);
            if (iSize == INVALID_FILE_SIZE)
                break;
            SetFilePointer(h, 0, NULL, FILE_BEGIN);
            *buf = malloc(iSize);
            if (NULL == *buf)
                break;
            DWORD iReaded = 0;
            if (ReadFile(h, *buf, iSize, &iReaded, NULL))
            {
                bufsize = iSize;
                bSuccess = true;
            }
        } while (0);
        CloseHandle(h);
    }
    return bSuccess;
}

bool SaveBufferToFile(const std::wstring &file, const void *buf, DWORD bufsize)
{
    if (NULL == buf || bufsize == 0)
        return false;
    bool bSuccess = false;
    HANDLE h = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            SetFilePointer(h, 0, NULL, FILE_BEGIN);
            DWORD wted = 0;
            if (TRUE == WriteFile(h, buf, bufsize, &wted, NULL))
            {
                bSuccess = true;
            }
        } while (0);
        CloseHandle(h);
    }
    return bSuccess;
}


MemoryStream::MemoryStream()
    :m_buffer(NULL), m_size(0), m_pos(0), m_capacity(0), m_init_capacity(1024)
{

}

MemoryStream::~MemoryStream()
{
    Clear();
}

long MemoryStream::Read(MemoryStream &dest, long bytes)
{
    long new_bytes = min(m_size - m_pos, bytes);
    if (new_bytes <= 0)
        return 0;
    void *p = m_buffer;
    IncPtr(&p, m_pos);
    dest.Write(p, new_bytes);
    m_pos += new_bytes;
    return new_bytes;
}

long MemoryStream::Read(void *dest, long bytes)
{
    if (bytes <= 0)
        return 0;
    long ret = min(bytes, m_size - m_pos);
    if (ret <= 0)
        return 0;
    void *p = m_buffer;
    IncPtr(&p, m_pos);
    memcpy(dest, p, ret);
    m_pos += ret;
    return ret;
}

bool MemoryStream::Write(const MemoryStream &from, long bytes /*= -1*/)
{
    if (bytes < -1 || bytes == 0 || bytes > from.GetSize())
        return false;
    long lsize = 0;
    if (-1 == bytes)
        lsize = from.GetSize();
    else
        lsize = bytes;
    long add_size = lsize - (m_capacity - m_pos);
    while (add_size > 0)
    {
        if (!Expand())
            return false;
        add_size = lsize - (m_capacity - m_pos);
    }
    void *p = m_buffer;
    IncPtr(&p, m_pos);
    memcpy(p, from.GetBuffer(), lsize);
    m_pos += lsize;
    m_size = max(m_size, m_pos);
    return true;
}

bool MemoryStream::Write(const void *from, long bytes)
{
    if (bytes <= 0)
        return false;
    long add_size = bytes - (m_capacity - m_pos);
    while (add_size > 0)
    {
        if (!Expand())
            return false;
        add_size = bytes - (m_capacity - m_pos);
    }
    void *p = m_buffer;
    IncPtr(&p, m_pos);
    memcpy(p, from, bytes);
    m_pos += bytes;
    m_size = max(m_size, m_pos);
    return true;
}

void MemoryStream::Seek(SeekOrigin so, long offset)
{
    if (m_size <= 0)
        return;
    long os = 0;
    if (so == soBegin)
        os = offset;
    else if (so == soEnd)
        os = m_size - 1 + offset;
    else if (so == soCurrent)
        os = m_pos + offset;
    else
        return;
    if (os < 0)
        os = 0;
    else if (os >= m_size)
        os = m_size - 1;
    m_pos = os;
}

void MemoryStream::Clear()
{
    free(m_buffer);
    m_buffer = NULL;
    m_pos = 0;
    m_size = 0;
    m_capacity = 0;
}

bool MemoryStream::LoadFromFile(const std::wstring &file)
{
    void *buf = NULL;
    DWORD bufsize = 0;
    if (LoadFileToBuffer(file, &buf, bufsize))
    {
        Clear();
        Write(buf, bufsize);
        free(buf);
        return true;
    }
    return false;
}

bool MemoryStream::SaveToFile(const std::wstring &file)
{
    return SaveBufferToFile(file, m_buffer, m_size);
}

bool MemoryStream::Expand(long new_capacity /*= -1*/)
{
    long newp = m_capacity * 2;
    if (0 == newp)
        newp = m_init_capacity;
    if (new_capacity != -1)
        newp = new_capacity;
    if (newp <= m_capacity)
        return false;
    void *new_buf = NULL;
    if (m_buffer == NULL)
        new_buf = malloc(newp);
    else
        new_buf = realloc(m_buffer, newp);
    if (NULL == new_buf)
        return false;
    m_buffer = new_buf;
    m_capacity = newp;
    return true;
}
