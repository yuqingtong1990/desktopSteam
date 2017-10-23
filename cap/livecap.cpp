// livecap.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "lc_audio_mix.h"
#include "lc_wasa_audio_cap.h"
#include "lc_faac.h"
#include "lc_x264.h"
#include "lc_yuv.h"
#include "lc_rtmpSend.h"
#include "lc_bitmap_source.h"

#define  FILE_MIC_PATH        "D://mic.wav"
#define  FILE_PLAYER_PATH     "D://player.wav"
#define  OUT_FILE_PATH        "D://mix.wav"
#define  OUT_FILE_AAC_PATH    "D://mix.aac"


 int APIENTRY _tWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR    lpCmdLine,
                      int       nCmdShow)
 {
	//音频采集
	lc_wasa_audio_cap capPlayer(Dev_Player);
	capPlayer.Start();
	capPlayer.Captrue(NULL);
	//音频编码
	WAVEFORMATEX* fpx =  capPlayer.getWavEformatex();
	lc_faac_encoder::get().Init(fpx->nSamplesPerSec,fpx->nChannels); 
	lc_faac_encoder::get().Start();


	//视频采集
	lc_bitmap_destop bmpDestop;
	bmpDestop.Start();

	//视频编码
	lc_x264_encoder::get().Init(1920,1080);
	lc_x264_encoder::get().Start();

	lc_rtmpsend::get().Init();
	lc_rtmpsend::get().Connect("rtmp://publish3.cdn.ucloud.com.cn/ucloud/123456");
	lc_rtmpsend::get().Start();
	int nFrames = 5000;
	while (nFrames)
	{
		HANDLE h = CreateEvent(NULL, FALSE, FALSE, NULL);
		WaitForSingleObject(h, 100);
		nFrames--;
	}
	capPlayer.Stop();
	lc_faac_encoder::get().Stop();
	lc_x264_encoder::get().Stop();
	bmpDestop.Stop();
	lc_rtmpsend::get().Stop();
	
 	return 0;
 }

// 
// lc_wasa_audio_cap capPlayer(Dev_Player);
// lc_wasa_audio_cap capMic(Dev_Microphone);
// capPlayer.Start();
// capMic.Start();
// 
// capPlayer.Captrue(FILE_PLAYER_PATH);
// capMic.Captrue(FILE_MIC_PATH);
// capMic.SetCallBack(lc_audio_mixing::FillAudioData);
// capPlayer.SetCallBack(lc_audio_mixing::FillAudioData);
// //     CWaveFile wavfile;
// //     wavfile.Create(OUT_FILE_PATH, capPlayer.getWavEformatex());
// FILE* f = NULL;
// fopen_s(&f, OUT_FILE_PATH, "wb+");
// lc_audio_mixing::get().SetFile(f);
// int nFrames = 500;
// while (nFrames)
// {
// 	HANDLE h = CreateEvent(NULL, FALSE, FALSE, NULL);
// 	WaitForSingleObject(h, 100);
// 	nFrames--;
// }
// // wavfile.Close();
// fclose(f);
// capPlayer.Stop();
// capMic.Stop();
// return 0;