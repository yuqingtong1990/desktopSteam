#pragma once
#include <Windows.h>
#include <stdint.h>
#include <string>


void InitGdiplus();
void UnInitGdiplus();

void GetDestopAsBmp(void** bmp, int* iWidth, int* iHeight);
void ClearBmp();
void ConvertRGB24YUVI420(int32_t w, int32_t h, uint8_t *bmp, uint8_t *yuv);
bool ConvertRGB2YUV(int32_t w, int32_t h, uint8_t* rgbData, uint8_t* y, uint8_t* u, uint8_t*v);
bool WriteYuv(FILE* f, int32_t w, int32_t h, uint8_t * y, uint8_t * u, uint8_t *v);
bool WriteYuv(FILE* f, int32_t w, int32_t h, uint8_t * yuv);
bool SaveBitmapAsfmt(HBITMAP hBitmap, WCHAR* fmt, LPCTSTR lpFileName);

typedef struct PacketDataTime
{
	void* pbuffer;
	uint32_t buffersize;
	int64_t timeTicket;

	void release(){
		free(pbuffer);
		pbuffer = NULL;
		buffersize = 0;
	}
}PDT;


class AutoLock
{
public:
    AutoLock(){};
    //cri要先调用InitializeCriticalSection初始化，不使用要了调用DeleteCriticalSection
    explicit AutoLock(CRITICAL_SECTION* cri){ f_lock = cri; EnterCriticalSection(f_lock); }
    ~AutoLock(){ LeaveCriticalSection(f_lock); }
private:
    CRITICAL_SECTION *f_lock;
    AutoLock(const AutoLock&);
    AutoLock& operator=(const AutoLock&);
};

class MemoryStream
{
public:
    enum SeekOrigin{ soBegin, soEnd, soCurrent };
    MemoryStream(void);
    virtual ~MemoryStream(void);
public:
    long Read(MemoryStream &dest, long bytes);
    long Read(void *dest, long bytes);
    bool Write(const MemoryStream &from, long bytes = -1);
    bool Write(const void *from, long bytes);
    long GetSize() const { return m_size; }
    long GetPos() const { return m_pos; }
    long GetCapacity() const { return m_capacity; }
    void* GetBuffer() const { return m_buffer; }
    void Seek(SeekOrigin so, long offset);
    void Clear();
    bool SaveToFile(const std::wstring &file);
    bool LoadFromFile(const std::wstring &file);
    bool Expand(long new_capacity = -1);//-1 默认为当前容量的两倍
    void SetInitCapacity(long cc){ m_init_capacity = cc; }
private:
    MemoryStream(const MemoryStream&);
    MemoryStream& operator=(const MemoryStream&);
private:
    void* m_buffer;
    long m_size;
    long m_pos;
    long m_capacity;
    long m_init_capacity;
};