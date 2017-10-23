#include "stdafx.h"
#include "lc_x264.h"

#pragma comment(lib,"libx264.lib")

lc_x264_encoder::lc_x264_encoder()
    :x264(NULL)
{

	m_hEventStop= CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hEventReady = CreateEvent(NULL,FALSE,FALSE,NULL);

	InitializeCriticalSection(&m_HaveSection); 
	InitializeCriticalSection(&m_PreSection); 
}

lc_x264_encoder::~lc_x264_encoder()
{
	::DeleteCriticalSection(&m_HaveSection);
    ::DeleteCriticalSection(&m_PreSection);
    x264_picture_clean(pPic_in);
    x264_encoder_close(x264);
    x264 = NULL;
    free(pPic_in);
    free(pPic_out);
}

void lc_x264_encoder::Init(int i_width, int i_height, int profile /*= profile_baseline*/, int csp /*= X264_CSP_I420*/)
{
	x264_param_default(&paramData);
	x264_param_default_preset(&paramData, "superfast", "zerolatency");
	paramData.b_sliced_threads = 0; //
	paramData.i_threads = 1;
	paramData.i_width = i_width;
	paramData.i_height = i_height;
	paramData.i_csp = csp;

	if (profile >profile_high444)
	{
		profile = profile_high444;
	}
	x264_param_apply_profile(&paramData, x264_profile_names[profile]);


	paramData.i_keyint_max = 10;      //  
	paramData.b_repeat_headers = 0;
	paramData.i_bframe = 0;   // 	    
	paramData.i_fps_den = 1;   // 
	paramData.i_fps_num = 15;  // 
	paramData.i_timebase_den = paramData.i_fps_num; //
	paramData.i_timebase_num = paramData.i_fps_den;
	paramData.rc.i_bitrate = 4096 * 1000 / 1000;  //  
	paramData.rc.i_rc_method = X264_RC_ABR;

	x264 = x264_encoder_open(&paramData);

	pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_picture_init(pPic_in);

	x264_picture_alloc(pPic_in, paramData.i_csp, paramData.i_width, paramData.i_height);

	//获取sps pps信息
	x264_nal_t *pNal;
	int iNal;
	x264_encoder_headers(x264, &pNal, &iNal);
	header_.Clear();
	for (int j = 0; j < iNal; ++j){
		header_.Write(pNal[j].p_payload, pNal[j].i_payload);
		PDT pdt;
		pdt.pbuffer = malloc( pNal[j].i_payload);
		pdt.buffersize =pNal[j].i_payload;
		memcpy(pdt.pbuffer,pNal[j].p_payload,pNal[j].i_payload);
		pdt.timeTicket = GetTickCount64();
		m_vecheader.push_back(pdt);
	}
}

void lc_x264_encoder::Start()
{
	CloseHandle((HANDLE)_beginthreadex(NULL, 0, &lc_x264_encoder::EncodeThread, (void*)this, 0, NULL));
}

void lc_x264_encoder::Stop()
{
	SetEvent(m_hEventStop);
}

PDT lc_x264_encoder::getPdt()
{
	AutoLock autolock(&m_HaveSection);
	PDT pdt;
	if (m_vecheader.empty())
	{
		pdt = m_vecHaveEncode.front();
		m_vecHaveEncode.erase(m_vecHaveEncode.begin());
	}
	else
	{
		pdt = m_vecheader.front();	
		m_vecheader.erase(m_vecheader.begin());
	}
	
	return pdt;
}

int64_t lc_x264_encoder::getFirstFrameTime()
{
	AutoLock autolock(&m_HaveSection);
	if (m_vecheader.empty())
	{
		if (m_vecHaveEncode.empty())
		{
			return 0;
		}
		else
		{
			m_vecHaveEncode.front().timeTicket;
		}
	}
	else
	{
		m_vecheader.front().timeTicket;
	}
	
}

void lc_x264_encoder::GetHeader(void** ppheader, int* pisize)
{
    *ppheader = header_.GetBuffer();
    *pisize = header_.GetSize();
}


bool lc_x264_encoder::Encodelst(std::vector<YUV> framelst, void** frame, int* fsize)
{
    *frame = NULL;
    *fsize = 0;

    MemoryStream stream_;
    for (unsigned int i = 0; i < framelst.size(); i++){
        int y_size = framelst[i].width * framelst[i].height;
        memcpy(pPic_in->img.plane[0], framelst[i].yBuffer, y_size);
        memcpy(pPic_in->img.plane[1], framelst[i].uBuffer, y_size / 4);
        memcpy(pPic_in->img.plane[2], framelst[i].vBuffer, y_size / 4);
        pPic_in->i_pts = i;

        x264_nal_t *pNals;
        int iNal;

        int ret = x264_encoder_encode(x264, &pNals, &iNal, pPic_in, pPic_out);       
        if (ret < 0){
            printf("Error.\n");
            return false;                                                         
        }


        for (int j = 0; j < iNal; ++j){                                            
            stream_.Write(pNals[j].p_payload, pNals[j].i_payload);
        }
    }

    if (stream_.GetSize() <= 0)
    {
        return false;
    }

    *frame = malloc(stream_.GetSize());
    if (*frame)
    {
        stream_.Seek(MemoryStream::soBegin, 0);
        stream_.Read(*frame, stream_.GetSize());
        *fsize = stream_.GetSize();
    }
    return true;
}

/*  
    IDR图像 首个I帧 DR（Instantaneous Decoding Refresh）--即时解码刷新。
    NAL_UNKNOWN = 0,
    NAL_SLICE = 1,          //一个非IDR图像的编码条带 
    NAL_SLICE_DPA = 2,      //编码条带数据分割块A
    NAL_SLICE_DPB = 3,      //编码条带数据分割块B 
    NAL_SLICE_DPC = 4,      //编码条带数据分割块C 
    NAL_SLICE_IDR = 5,      //IDR图像的编码条带 
    NAL_SEI = 6,            //辅助增强信息 (SEI) 
    NAL_SPS = 7,            //序列参数集  SPS
    NAL_PPS = 8,            //图像参数集  PPS
    NAL_AUD = 9,            //访问单元分隔符 
    NAL_FILLER = 12,        //填充数据 

    10 序列结尾
    11 流结尾
    13 序列参数集扩展
*/
bool lc_x264_encoder::EncodeOne(const YUV& yuv, PDT& pdt)
{
	pdt.buffersize = 0;
	pdt.pbuffer = nullptr;
	pdt.timeTicket = GetTickCount64();

	MemoryStream stream_;

	int y_size = yuv.width * yuv.height;
	memcpy(pPic_in->img.plane[0], yuv.yBuffer, y_size);
	memcpy(pPic_in->img.plane[1], yuv.uBuffer, y_size / 4);
	memcpy(pPic_in->img.plane[2], yuv.vBuffer, y_size / 4);
	pPic_in->i_pts = pdt.timeTicket;//这里不支持xp

	x264_nal_t *pNals;
	int iNal;

	int ret = x264_encoder_encode(x264, &pNals, &iNal, pPic_in, pPic_out);       
	if (ret < 0){
		printf("Error.\n");
		return false;                                                         
	}

	for (int j = 0; j < iNal; ++j){                                            
		stream_.Write(pNals[j].p_payload, pNals[j].i_payload);
	}

	if (stream_.GetSize() <= 0)
	{
		return false;
	}

	pdt.pbuffer = malloc(stream_.GetSize());
	if (pdt.pbuffer)
	{
		stream_.Seek(MemoryStream::soBegin, 0);
		stream_.Read(pdt.pbuffer, stream_.GetSize());
		pdt.buffersize = stream_.GetSize();
	}
	return true;
}

void lc_x264_encoder::AddtoEncodeLst(const YUV& yuv)
{
	AutoLock lock(&m_PreSection);
	m_vecWaitEncode.push_back(yuv);
	SetEvent(m_hEventReady);
}

void lc_x264_encoder::EncodeProcess()
{
	HANDLE waitArray[2] = { m_hEventStop, m_hEventReady };
	while (true)
	{   
		DWORD result = WaitForMultipleObjects(2, waitArray, FALSE, 1000); 
		if (result == WAIT_OBJECT_0)
		{
			break;
		}
		else if (result == WAIT_OBJECT_0 + 1 || result == WAIT_TIMEOUT)
		{
			while(true)
			{
				YUV pic_org;
				{
					AutoLock autolock(&m_PreSection);
					if (m_vecWaitEncode.empty())
						break;

					pic_org = m_vecWaitEncode.front();
					m_vecWaitEncode.erase(m_vecWaitEncode.begin());

				}
				PDT pdt;
				EncodeOne(pic_org,pdt);
				pic_org.clear();

				{
					AutoLock autolock(&m_HaveSection);
					m_vecHaveEncode.push_back(pdt);
				}

			}
		}  
	}
}


lc_x264_encoder& lc_x264_encoder::get()
{
	static lc_x264_encoder s_instance;
	return s_instance;
}

unsigned int WINAPI lc_x264_encoder::EncodeThread(void* param)
{
	lc_x264_encoder* pThis = (lc_x264_encoder*)param;
	if (pThis)
	{
		pThis->EncodeProcess();
	}
	return 0;
}
