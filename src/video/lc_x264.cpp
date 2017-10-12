#include "stdafx.h"
#include "lc_x264.h"

#pragma comment(lib,"libx264.lib")

lc_x264_encoder::lc_x264_encoder(int i_width, int i_height, int profile/* = profile_high*/,int csp/* = X264_CSP_I420*/)
    :x264(NULL)
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
    
    //��ȡsps pps��Ϣ
    x264_nal_t *pNal;
    int iNal;
    x264_encoder_headers(x264, &pNal, &iNal);
    header_.Clear();
    for (int j = 0; j < iNal; ++j){
        header_.Write(pNal[j].p_payload, pNal[j].i_payload);
    }
}

lc_x264_encoder::~lc_x264_encoder()
{
    x264_picture_clean(pPic_in);
    x264_encoder_close(x264);
    x264 = NULL;
    free(pPic_in);
    free(pPic_out);
}

void lc_x264_encoder::GetHeader(void** ppheader, int* pisize)
{
    *ppheader = header_.GetBuffer();
    *pisize = header_.GetSize();
}


bool lc_x264_encoder::Encode(std::vector<YUV> framelst, void** frame, int* fsize)
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


        /*  
            IDRͼ�� �׸�I֡ DR��Instantaneous Decoding Refresh��--��ʱ����ˢ�¡�
            NAL_UNKNOWN = 0,
            NAL_SLICE = 1,          //һ����IDRͼ��ı������� 
            NAL_SLICE_DPA = 2,      //�����������ݷָ��A
            NAL_SLICE_DPB = 3,      //�����������ݷָ��B 
            NAL_SLICE_DPC = 4,      //�����������ݷָ��C 
            NAL_SLICE_IDR = 5,      //IDRͼ��ı������� 
            NAL_SEI = 6,            //������ǿ��Ϣ (SEI) 
            NAL_SPS = 7,            //���в�����  SPS
            NAL_PPS = 8,            //ͼ�������  PPS
            NAL_AUD = 9,            //���ʵ�Ԫ�ָ��� 
            NAL_FILLER = 12,        //������� 

            10 ���н�β
            11 ����β
            13 ���в�������չ
        */
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
