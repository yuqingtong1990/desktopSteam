#include "StdAfx.h"
#include "lc_wavefile.h"


#define MM_SUCC(x) (MMSYSERR_NOERROR == (x))
#define MM_FAILED(x) (MMSYSERR_NOERROR != (x))
#define CHECK_MMERR(x) CHECK_BOOL(MM_SUCC(x))
#define CHECK_BOOL(x) {if (!(x)) return FALSE; }


static FOURCC g_fccWave = mmioFOURCC('W', 'A', 'V', 'E');
static FOURCC g_fccFmt = mmioFOURCC('f', 'm', 't', ' ');
static FOURCC g_fccFact = mmioFOURCC('f', 'a', 'c', 't');
static FOURCC g_fccData = mmioFOURCC('d', 'a', 't', 'a');

lc_wavefile::lc_wavefile()
{
	m_hmmio = 0;
	m_nStatus = status_close;
	m_dwSamples = 0;
}

lc_wavefile::~lc_wavefile()
{    
}

BOOL lc_wavefile::Create(char* sFileName, WAVEFORMATEX * pFmt)
{
	Close();

	m_hmmio = mmioOpenA((char*) sFileName, NULL,MMIO_CREATE | MMIO_READWRITE | MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
	CHECK_BOOL(m_hmmio);

	m_ckRIFF.fccType = g_fccWave;
	m_ckRIFF.cksize  = 0L;
	CHECK_MMERR(mmioCreateChunk(m_hmmio, &m_ckRIFF, MMIO_CREATERIFF));

	LONG lFmtSize = sizeof WAVEFORMATEX + pFmt->cbSize;
	MMCKINFO ckFmt;
	ckFmt.ckid   = g_fccFmt;
	ckFmt.cksize = 0L;
	CHECK_MMERR(mmioCreateChunk(m_hmmio, &ckFmt, 0));
	CHECK_BOOL(lFmtSize == mmioWrite(m_hmmio, (HPSTR)pFmt, lFmtSize));
	CHECK_MMERR(mmioAscend(m_hmmio, &ckFmt, 0));

	m_ckFact.ckid   = g_fccFact;
	m_ckFact.cksize = 0L;
	CHECK_MMERR(mmioCreateChunk(m_hmmio, &m_ckFact, 0));
	m_dwSamples = 0L;
	CHECK_BOOL(sizeof DWORD == mmioWrite(m_hmmio, (HPSTR)&m_dwSamples, sizeof DWORD));
	CHECK_MMERR(mmioAscend(m_hmmio, &m_ckFact, 0));

	m_ckData.ckid   = g_fccData;
	m_ckData.cksize = 0L;
	CHECK_MMERR(mmioCreateChunk(m_hmmio, &m_ckData, 0));

	m_nStatus = status_write;
	return TRUE;
}

BOOL lc_wavefile::WriteData(void * pData, DWORD dwSize, DWORD dwSamples)
{
	CHECK_BOOL(m_hmmio);
	CHECK_BOOL((LONG) dwSize == mmioWrite(m_hmmio, (HPSTR)pData, dwSize));
	m_dwSamples += dwSamples;
	return TRUE;
}

int lc_wavefile::Seek(int iOffset, tSeekOrigin origin)
{
	CHECK_BOOL(m_hmmio);
	if (seek_begin == origin)
		return mmioSeek(m_hmmio, iOffset + m_ckData.dwDataOffset, SEEK_SET);
	else if (seek_current == origin)
		return mmioSeek(m_hmmio, iOffset, SEEK_CUR);
	else if (seek_current == origin)
		return mmioSeek(m_hmmio, iOffset + m_ckData.dwDataOffset + m_ckData.cksize, SEEK_SET);
	else;
	return mmioSeek(m_hmmio, 0, SEEK_CUR);
}

void lc_wavefile::Close()
{
	if (m_hmmio)
	{
		if (status_write == m_nStatus)
		{
			mmioAscend(m_hmmio, &m_ckData, 0);
			mmioAscend(m_hmmio, &m_ckRIFF, 0);
			mmioSeek(m_hmmio, m_ckFact.dwDataOffset, SEEK_SET);
			mmioWrite(m_hmmio, (HPSTR)&m_dwSamples, sizeof(DWORD));
		}
		else if (status_read == m_nStatus)
		{
			mmioAscend(m_hmmio, &m_ckData, 0);
			mmioAscend(m_hmmio, &m_ckRIFF, 0);
		}

		mmioClose(m_hmmio, 0);
		m_hmmio = 0;
		m_nStatus = status_close;
		m_dwSamples = 0;
	}
}
