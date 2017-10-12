#pragma once
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define    tStatus        enum _enumStatus
#define    tSeekOrigin    enum _enumSeekOrigin
class lc_wavefile  
{
public:
	enum _enumStatus{
		status_close = 0,
		status_write,
		status_read,
	};

	enum _enumSeekOrigin{
		seek_begin,
		seek_current,
		seek_end,
	};

public:
	lc_wavefile();
	virtual ~lc_wavefile();

public:
	BOOL Create(char* sFileName, WAVEFORMATEX * pFmt);
	void Close();

	BOOL WriteData(void * pData, DWORD dwSize, DWORD dwSamples = 0);
	int Seek(int iOffset, tSeekOrigin origin = seek_begin);

	DWORD GetSampleCount() { return m_dwSamples; }

protected:
	HMMIO m_hmmio;
	tStatus m_nStatus;
	MMCKINFO m_ckRIFF;
	MMCKINFO m_ckData;
	MMCKINFO m_ckFact;
	DWORD m_dwSamples;
};
