#include "lc_rtmpSend.h"
#include "lc_x264.h"
#include "lc_faac.h"

#include <math.h>
#include <WinSock2.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib") 
#pragma comment(lib,"librtmp.lib")
typedef enum {   
	AVC_NULL			= 0,	
	AVC_SPS_INIT		= 1,	
	AVC_DIR_INIT		= 2,
}AVC_PRO_TYPE;

bool  hasStartCode(const uint8_t *pdata)
{
	if (pdata[0] != 0 || pdata[1] != 0)
		return FALSE;

	return pdata[2] == 1 || (pdata[2] == 0 && pdata[3] == 1);
}

/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
* scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
* we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
static const uint8_t*  ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) {
		uint32_t x = *(const uint32_t*)p;

		if ((x - 0x01010101) & (~x) & 0x80808080) {
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}

			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

static const uint8_t*  avcFindStartcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out = ff_avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1]) out--;
	return out;
}

uint32_t  Ue(uint8_t *pBuff, uint32_t nLen, uint32_t* nStartBit)
{
	//计算0bit的个数
	uint32_t nZeroNum = 0; uint32_t i = 0;
	unsigned long dwRet = 0;
	while ((*nStartBit) < nLen * 8)
	{
		if (pBuff[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8))) 
		{
			break;
		}
		nZeroNum++;
		(*nStartBit)++;
	}
	(*nStartBit)++;

	//计算结果
	for (  i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))
		{
			dwRet += 1;
		}
		(*nStartBit)++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}

int  Se(uint8_t *pBuff, uint32_t nLen, uint32_t* nStartBit)
{
	int UeVal = Ue(pBuff, nLen, (nStartBit));
	double k = UeVal;
	int nValue =  ceil(k / 2);
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}

unsigned long  u(uint32_t BitCount, uint8_t * buf, uint32_t* nStartBit)
{
	unsigned long dwRet = 0;uint32_t i = 0;
	for (  i = 0; i < BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))
		{
			dwRet += 1;
		}
		(*nStartBit)++;
	}
	return dwRet;
}

/**
* H264的NAL起始码防竞争机制
*
* @param buf SPS数据内容
* @无返回值 */
void  de_emulation_prevention(uint8_t* buf, uint32_t* buf_size)
{
	uint32_t i = 0, j = 0;
	uint8_t* tmp_ptr = NULL;
	uint32_t tmp_buf_size = 0;
	int val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i < (tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j < tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}


int  h264_decode_sps(uint8_t * buf, uint32_t nLen, uint32_t* width, uint32_t* height, uint32_t* fps)
{
	uint32_t StartBit = 0; int i = 0;
	*fps = 0;
	de_emulation_prevention(buf, &nLen);

	int forbidden_zero_bit;
	int nal_ref_idc;
	int nal_unit_type;
	int profile_idc;
	int constraint_set0_flag;
	int constraint_set1_flag;
	int constraint_set2_flag;
	int constraint_set3_flag;
	int reserved_zero_4bits;
	int level_idc;
	int seq_parameter_set_id;
	int chroma_format_idc;
	int residual_colour_transform_flag;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	int qpprime_y_zero_transform_bypass_flag;
	int seq_scaling_matrix_present_flag;
	int seq_scaling_list_present_flag[8];
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;
	int frame_cropping_flag;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;
	int vui_parameter_present_flag;
	int aspect_ratio_info_present_flag;
	int aspect_ratio_idc;
	int sar_width;
	int sar_height;
	int overscan_info_present_flag;
	int overscan_appropriate_flagu;
	int video_signal_type_present_flag;
	int video_format;
	int video_full_range_flag;
	int colour_description_present_flag;
	int colour_primaries;
	int transfer_characteristics;
	int matrix_coefficients;
	int chroma_loc_info_present_flag;
	int chroma_sample_loc_type_top_field;
	int chroma_sample_loc_type_bottom_field;
	int timing_info_present_flag;
	int num_units_in_tick;
	int time_scale;

	forbidden_zero_bit = u(1, buf, &StartBit);
	nal_ref_idc = u(2, buf, &StartBit);
	nal_unit_type = u(5, buf, &StartBit);
	if (nal_unit_type == 7)
	{
		profile_idc = u(8, buf, &StartBit);
		constraint_set0_flag = u(1, buf, &StartBit);//(buf[1] & 0x80)>>7;
		constraint_set1_flag = u(1, buf, &StartBit);//(buf[1] & 0x40)>>6;
		constraint_set2_flag = u(1, buf, &StartBit);//(buf[1] & 0x20)>>5;
		constraint_set3_flag = u(1, buf, &StartBit);//(buf[1] & 0x10)>>4;
		reserved_zero_4bits = u(4, buf, &StartBit);
		level_idc = u(8, buf, &StartBit);

		seq_parameter_set_id = Ue(buf, nLen, &StartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			chroma_format_idc = Ue(buf, nLen, &StartBit);
			if (chroma_format_idc == 3)
				residual_colour_transform_flag = u(1, buf, &StartBit);
			bit_depth_luma_minus8 = Ue(buf, nLen, &StartBit);
			bit_depth_chroma_minus8 = Ue(buf, nLen, &StartBit);
			qpprime_y_zero_transform_bypass_flag = u(1, buf, &StartBit);
			seq_scaling_matrix_present_flag = u(1, buf, &StartBit);

			if (seq_scaling_matrix_present_flag)
			{
				for (  i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, &StartBit);
				}
			}
		}
		log2_max_frame_num_minus4 = Ue(buf, nLen, &StartBit);
		pic_order_cnt_type = Ue(buf, nLen, &StartBit);
		if (pic_order_cnt_type == 0)
			log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, &StartBit);
		else if (pic_order_cnt_type == 1)
		{
			delta_pic_order_always_zero_flag = u(1, buf, &StartBit);
			offset_for_non_ref_pic = Se(buf, nLen, &StartBit);
			offset_for_top_to_bottom_field = Se(buf, nLen, &StartBit);
			num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, &StartBit);

			int *offset_for_ref_frame = (int*)malloc(num_ref_frames_in_pic_order_cnt_cycle);
			for (  i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, &StartBit);
			free(offset_for_ref_frame);
		}
		num_ref_frames = Ue(buf, nLen, &StartBit);
		gaps_in_frame_num_value_allowed_flag = u(1, buf, &StartBit);
		pic_width_in_mbs_minus1 = Ue(buf, nLen, &StartBit);
		pic_height_in_map_units_minus1 = Ue(buf, nLen, &StartBit);

		frame_mbs_only_flag = u(1, buf, &StartBit);
		if (!frame_mbs_only_flag)
			mb_adaptive_frame_field_flag = u(1, buf, &StartBit);

		direct_8x8_inference_flag = u(1, buf, &StartBit);
		frame_cropping_flag = u(1, buf, &StartBit);
		if (frame_cropping_flag)
		{
			frame_crop_left_offset = Ue(buf, nLen, &StartBit);
			frame_crop_right_offset = Ue(buf, nLen, &StartBit);
			frame_crop_top_offset = Ue(buf, nLen, &StartBit);
			frame_crop_bottom_offset = Ue(buf, nLen, &StartBit);
		}

		//计算视频分辨率,有的视频尺寸可能不是完整划分,所以需要offset计算
		*width = (pic_width_in_mbs_minus1 + 1) * 16;
		*height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
		if (frame_cropping_flag)
		{
			uint32_t crop_unit_x;
			uint32_t crop_unit_y;
			if (0 == chroma_format_idc) // monochrome
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			else if (1 == chroma_format_idc) // 4:2:0
			{
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (2 == chroma_format_idc) // 4:2:2
			{
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			else // 3 == chroma_format_idc	// 4:4:4
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}

			*width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
			*height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
		}

		vui_parameter_present_flag = u(1, buf, &StartBit);
		if (vui_parameter_present_flag)
		{
			aspect_ratio_info_present_flag = u(1, buf, &StartBit);
			if (aspect_ratio_info_present_flag)
			{
				aspect_ratio_idc = u(8, buf, &StartBit);
				if (aspect_ratio_idc == 255)
				{
					sar_width = u(16, buf, &StartBit);
					sar_height = u(16, buf, &StartBit);
				}
			}
			overscan_info_present_flag = u(1, buf, &StartBit);
			if (overscan_info_present_flag)
				overscan_appropriate_flagu = u(1, buf, &StartBit);
			video_signal_type_present_flag = u(1, buf, &StartBit);
			if (video_signal_type_present_flag)
			{
				video_format = u(3, buf, &StartBit);
				video_full_range_flag = u(1, buf, &StartBit);
				colour_description_present_flag = u(1, buf, &StartBit);
				if (colour_description_present_flag)
				{
					colour_primaries = u(8, buf, &StartBit);
					transfer_characteristics = u(8, buf, &StartBit);
					matrix_coefficients = u(8, buf, &StartBit);
				}
			}
			chroma_loc_info_present_flag = u(1, buf, &StartBit);
			if (chroma_loc_info_present_flag)
			{
				chroma_sample_loc_type_top_field = Ue(buf, nLen, &StartBit);
				chroma_sample_loc_type_bottom_field = Ue(buf, nLen, &StartBit);
			}
			timing_info_present_flag = u(1, buf, &StartBit);
			if (timing_info_present_flag)
			{
				num_units_in_tick = u(32, buf, &StartBit);
				time_scale = u(32, buf, &StartBit);
				*fps = time_scale / (2 * num_units_in_tick);
			}
		}
		return TRUE;
	}
	else
		return FALSE;
}

enum
{
	FLV_CODECID_H264  = 7,
	AUDIO_CODECID_ACC = 10,
};

//处理字节
char * put_byte( char *output, uint8_t nVal )  
{  
	output[0] = nVal;  
	return output+1;  
}  
char * put_be16(char *output, uint16_t nVal )  
{  
	output[1] = nVal & 0xff;  
	output[0] = nVal >> 8;  
	return output+2;  
}  
char * put_be24(char *output,uint32_t nVal )  
{  
	output[2] = nVal & 0xff;  
	output[1] = nVal >> 8;  
	output[0] = nVal >> 16;  
	return output+3;  
}  
char * put_be32(char *output, uint32_t nVal )  
{  
	output[3] = nVal & 0xff;  
	output[2] = nVal >> 8;  
	output[1] = nVal >> 16;  
	output[0] = nVal >> 24;  
	return output+4;  
}  
char *  put_be64( char *output, uint64_t nVal )  
{  
	output=put_be32( output, nVal >> 32 );  
	output=put_be32( output, nVal );  
	return output;  
}  
char * put_amf_string( char *c, const char *str )  
{  
	uint16_t len = strlen( str );  
	c=put_be16( c, len );  
	memcpy(c,str,len);  
	return c+len;  
}  
char * put_amf_double( char *c, double d )  
{  
	*c++ = AMF_NUMBER;  /* type: Number */  
	{  
		unsigned char *ci, *co;  
		ci = (unsigned char *)&d;  
		co = (unsigned char *)c;  
		co[0] = ci[7];  
		co[1] = ci[6];  
		co[2] = ci[5];  
		co[3] = ci[4];  
		co[4] = ci[3];  
		co[5] = ci[2];  
		co[6] = ci[1];  
		co[7] = ci[0];  
	}  
	return c+8;  
}

typedef enum {
	NALU_TYPE_SLICE	= 1,
	NALU_TYPE_DPA		= 2,
	NALU_TYPE_DPB		= 3,
	NALU_TYPE_DPC		= 4,
	NALU_TYPE_IDR		= 5,
	NALU_TYPE_SEI		= 6,
	NALU_TYPE_SPS		= 7,
	NALU_TYPE_PPS		= 8,
	NALU_TYPE_AUD		= 9,
	NALU_TYPE_EOSEQ	= 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL 	= 12,

	NALU_TYPE_AUDIO	= 13,	 
} NaluType;


uint8_t get_h264_nalu_type(uint8_t* buf, int size,int* start_code_len){
	uint8_t nalu_type = 0;
	//int start_code_len = 0;

	if ((buf[0]==0) && (buf[1]==0) && (buf[2]==1)){
		*start_code_len = 3;
		nalu_type = (buf[3]&0x1f);
	}else if ((buf[0]==0) && (buf[1]==0) && (buf[2]==0) && (buf[3]==1)){
		*start_code_len = 4;
		nalu_type = (buf[4]&0x1f);
	}else{
		//Start code check fail
		*start_code_len = 0;
		printf(" start_code error .......  \n");
		return -1;
	}

	return nalu_type;
}

lc_rtmpsend::lc_rtmpsend()
	:bHaveSendSps_(false)
	,video_init(AVC_NULL)
{

}

lc_rtmpsend::~lc_rtmpsend()
{

}

void lc_rtmpsend::Init()
{
	WORD version;	
	WSADATA wsaData;  
	version = MAKEWORD(1, 1);	
	::WSAStartup(version, &wsaData);  
}

void lc_rtmpsend::Close()
{
	bHaveSendSps_ = false;
	if(m_pRtmp)
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
	}
}

#define RTMP_HEAD_SIZE	 (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

int lc_rtmpsend::SendVideoSpsPps(unsigned char *pps,int pps_len,unsigned char * sps,int sps_len)
{
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet,0,RTMP_HEAD_SIZE+1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;   ////   0x01;  AVC NALU         //  0x00    AVC sequence header

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i],pps,pps_len);
	i +=  pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;

	/*调用发送接口*/
	int nRet = RTMP_SendPacket(m_pRtmp,packet,TRUE);
	free(packet);    //释放内存
	return nRet;
}

bool lc_rtmpsend::SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp)
{
	if(data == NULL && size<11)
	{
		return 0;
	}

	//unsigned char *body = new unsigned char[size+9];
	unsigned char *body =   (unsigned char*) malloc(size+9);

	int i = 0;
	if(bIsKeyFrame)
	{
		body[i++] = 0x17;// 1:Iframe  7:AVC
	}
	else
	{
		body[i++] = 0x27;// 2:Pframe  7:AVC
	}
	body[i++] = 0x01;// AVC NALU    //  0x00; // AVC sequence header
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	// NALU size
	body[i++] = size>>24;
	body[i++] = size>>16;
	body[i++] = size>>8;
	body[i++] = size&0xff;;

	// NALU data
	memcpy(&body[i],data,size);

	int bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);

	//delete[] body;
	free(body);
	return bRet > 0 ? true: false;
}

bool lc_rtmpsend::SendMetadata(LPRTMPMetadata lpMetaData)
{
	if(lpMetaData == NULL)
	{
		return 0;
	}
	char body[1024] = {0};;

	char * p = (char *)body;  
	p = put_byte(p, AMF_STRING );
	p = put_amf_string(p , "@setDataFrame" );

	p = put_byte( p, AMF_STRING );
	p = put_amf_string( p, "onMetaData" );

	p = put_byte(p, AMF_OBJECT );  
	p = put_amf_string( p, "copyright" );  
	p = put_byte(p, AMF_STRING );  
	p = put_amf_string( p, "tao_" );  

	p =put_amf_string( p, "width");
	p =put_amf_double( p, lpMetaData->nWidth);

	p =put_amf_string( p, "height");
	p =put_amf_double( p, lpMetaData->nHeight);

	p =put_amf_string( p, "framerate" );
	p =put_amf_double( p, lpMetaData->nFrameRate); 

	p =put_amf_string( p, "videocodecid" );
	p =put_amf_double( p, FLV_CODECID_H264 );

#if  1

	p =put_amf_string( p, "aacaot" );
	p =put_amf_double( p, 1); 

	//p =put_amf_string( p, "audiodatarate");
	//p =put_amf_double( p, lpMetaData->nAudioDatarate);

	p =put_amf_string( p, "audiosamplerate");
	p =put_amf_double( p, lpMetaData->nAudioSampleRate);

	p =put_amf_string( p, "audiosamplesize" );
	p =put_amf_double( p, lpMetaData->nAudioSampleSize); 

	p =put_amf_string( p, "stereo" );
	p =put_amf_double( p, lpMetaData->nAudioChannels); 


	p =put_amf_string( p, "audiocodecid" );
	p =put_amf_double( p, AUDIO_CODECID_ACC); 
#endif


	p =put_amf_string( p, "" );
	p =put_byte( p, AMF_OBJECT_END  );

	int index = p-body;

	SendPacket(RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);


	int i = 0;
	body[i++] = 0x17; // 1:keyframe  7:AVC   flv tag data 头
	body[i++] = 0x00; // AVC sequence header    ////  0x01;  AVC NALU 

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00; // fill in 0;

	// AVCDecoderConfigurationRecord.
	body[i++] = 0x01; // configurationVersion // 版本号
	body[i++] = lpMetaData->Sps[1]; // AVCProfileIndication
	body[i++] = lpMetaData->Sps[2]; // profile_compatibility
	body[i++] = lpMetaData->Sps[3]; // AVCLevelIndication 
	body[i++] = 0xff; // lengthSizeMinusOne  

	// sps nums
	body[i++] = 0xE1; //&0x1f
	// sps data length
	body[i++] = lpMetaData->nSpsLen>>8;
	body[i++] = lpMetaData->nSpsLen&0xff;

	// sps data 
	memcpy(&body[i],lpMetaData->Sps,lpMetaData->nSpsLen);
	i= i+lpMetaData->nSpsLen;

	// pps nums
	body[i++] = 0x01; //&0x1f
	// pps data length 
	body[i++] = lpMetaData->nPpsLen>>8;
	body[i++] = lpMetaData->nPpsLen&0xff;
	// sps data
	memcpy(&body[i],lpMetaData->Pps,lpMetaData->nPpsLen);
	i= i+lpMetaData->nPpsLen;

	printf("  enable sps pps \n");

	SendPacket(RTMP_PACKET_TYPE_VIDEO,(unsigned char*)body,i,0);////    //  missing picture in access unit with size 3267

	//  [h264 @ 0xb01042e0] no frame!
	// aac 
	i = 0;
	body[i++] = 0xAF; //  
	body[i++] = 0x00; //  
	uint16_t audio_specific_config=0;

	audio_specific_config |=((2<<11)&0xF800); // 2 : AAC LC (LOW Complexity)
	audio_specific_config |=((4<<7)&0x0780); // 4 : 44kHZ
	audio_specific_config |=((2<<3)&0x78); // 2 : Stereo
	audio_specific_config |=0 & 0x07;  //  padding:000

	body[i++] = (audio_specific_config>>8)&0xFF;
	body[i++] =  audio_specific_config & 0xFF;

	return SendPacket(RTMP_PACKET_TYPE_AUDIO,(unsigned char*)body,i,0);////    
}

int lc_rtmpsend::SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp)
{
	if(m_pRtmp == NULL)
	{
		return FALSE;
	}

	RTMPPacket packet;
	RTMPPacket_Reset(&packet);
	RTMPPacket_Alloc(&packet,size);

	packet.m_packetType = nPacketType;
	packet.m_nChannel = 0x04;  
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;  
	packet.m_nTimeStamp =   nTimestamp; // 0;  
	packet.m_nInfoField2 = m_pRtmp->m_stream_id;  
	packet.m_nBodySize = size;		 
	memcpy(packet.m_body,data,size);
	int nRet;
	if (RTMP_IsConnected(m_pRtmp)){  
		nRet = RTMP_SendPacket(m_pRtmp,&packet,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}		
	RTMPPacket_Free(&packet);
	return nRet;
}

bool lc_rtmpsend::SendAACPacket(unsigned char* data,unsigned int size,unsigned int nTimeStamp)
{
	if(m_pRtmp == NULL)
		return 0;

	if (size > 0){
		RTMPPacket * packet;
		unsigned char * body;

		packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size+2);
		memset(packet,0,RTMP_HEAD_SIZE);

		packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
		body = (unsigned char *)packet->m_body;

		/*AF 01 + AAC RAW data*/
		body[0] = 0xAF;
		body[1] = 0x01;
		memcpy(&body[2],data,size);

		packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		packet->m_nBodySize = size+2;
		packet->m_nChannel = 0x04;
		packet->m_nTimeStamp =   nTimeStamp;
		packet->m_hasAbsTimestamp = 0;
		packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
		packet->m_nInfoField2 = m_pRtmp->m_stream_id;

		/*调用发送接口*/
		RTMP_SendPacket(m_pRtmp,packet,TRUE);
		free(packet);
	}
	return 1;
}

lc_rtmpsend& lc_rtmpsend::get()
{
	static lc_rtmpsend s_instance;
	return s_instance;
}

void lc_rtmpsend::Start()
{
	 CloseHandle((HANDLE)_beginthreadex(NULL, 0, &lc_rtmpsend::SendThread, (void*)this, 0, NULL));
}

void lc_rtmpsend::Stop()
{
	::SetEvent(hEventStop);
}

void lc_rtmpsend::SendLoopProc()
{
	while (true)
	{   
		//50帧
		DWORD result = WaitForSingleObject(hEventStop,20);
		if (result == WAIT_OBJECT_0)
		{
			break;
		}
		else if (result == WAIT_TIMEOUT)
		{
			int64_t audiotime = lc_faac_encoder::get().getFirstFrameTime();
			int64_t videotime = lc_x264_encoder::get().getFirstFrameTime();

			if (audiotime == 0 || videotime == 0)
			{
				break;
			}

			//对比时间戳
			if (audiotime >= videotime)
			{
				//发送音频帧
				PDT pdta = lc_faac_encoder::get().getPdt();
				SendAACPacket((unsigned char*)pdta.pbuffer,  pdta.buffersize , pdta.timeTicket);	
			}
			else
			{
				//发送视频帧
				PDT pdtv = lc_x264_encoder::get().getPdt();
				int start_code_len = 0;
				   if(video_init != AVC_DIR_INIT)
				   {
						if(get_h264_nalu_type((uint8_t*)pdtv.pbuffer, pdtv.buffersize,&start_code_len)==NALU_TYPE_SPS && video_init==AVC_NULL)
						{ 
							metaData.nSpsLen = pdtv.buffersize-start_code_len;
							
							memcpy(metaData.Sps, (uint8_t*)(pdtv.pbuffer) + start_code_len , metaData.nSpsLen);// 开头的start code去掉 
							video_init=AVC_SPS_INIT;				  
							continue ;
						}
						if(get_h264_nalu_type((uint8_t*)pdtv.pbuffer, pdtv.buffersize,&start_code_len)==NALU_TYPE_SPS &&video_init==AVC_SPS_INIT){
							metaData.nPpsLen = pdtv.buffersize-start_code_len;
							memcpy(metaData.Pps, (uint8_t*)(pdtv.pbuffer) + start_code_len, metaData.nPpsLen);
							video_init=AVC_DIR_INIT;
							unsigned int width = 0,height = 0,fps=0;
							h264_decode_sps(metaData.Sps,metaData.nSpsLen,&width,&height,&fps);		   
							metaData.nWidth = width;
							metaData.nHeight = height;
							metaData.nFrameRate = fps;			  /////  帧率估计要修改    
			 
							metaData.nAudioSampleRate = 44100;
							metaData.nAudioChannels = 2;
							metaData.nAudioSampleSize = 0; /////   估计要修改   
							/*
							metaData.pAudioSpecCfg = height;
							metaData.nAudioSpecCfgLen = height;
							*/
							// 发送MetaData
							SendMetadata(&metaData);
						}
					}
					else
					{
						bool bKeyframe = (get_h264_nalu_type((uint8_t*)pdtv.pbuffer, pdtv.buffersize,&start_code_len)==NALU_TYPE_IDR) ? TRUE : FALSE;
						// 发送H264数据帧
						if(bKeyframe==1){
							SendVideoSpsPps(metaData.Pps,metaData.nPpsLen,metaData.Sps,metaData.nSpsLen);
						}
						SendH264Packet((uint8_t*)pdtv.pbuffer+start_code_len, pdtv.buffersize-start_code_len, bKeyframe,pdtv.timeTicket);	  
					}
			}

		}
	}
}

unsigned int WINAPI lc_rtmpsend::SendThread(void* param)
{
	lc_rtmpsend* pThis = (lc_rtmpsend*)param;
	if (pThis)
	{
		pThis->SendLoopProc();
	}
	return 0;
}

bool lc_rtmpsend::Connect(const char* url)
{
	if(RTMP_SetupURL(m_pRtmp, (char*)url)<0){
		return FALSE;
	}
	RTMP_EnableWrite(m_pRtmp);
	if(RTMP_Connect(m_pRtmp, NULL)<0){
		return FALSE;
	}
	if(RTMP_ConnectStream(m_pRtmp,0)<0){
		return FALSE;
	}
	return TRUE;
}
