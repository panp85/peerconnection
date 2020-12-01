#ifndef TEST_FFMPEG_H_
#define TEST_FFMPEG_H_

//#if defined(WEBRTC_LINUX)
//#include <unistd.h>
//#endif
#ifdef  __cplusplus
extern "C" {
#endif
#include "ffmpeg-api.h"
#ifdef  __cplusplus
}
#endif

#include <thread>

#include "api/video/video_rotation.h"
#include "api/video/video_sink_interface.h"
#include "modules/include/module.h"
#include "modules/video_capture/video_capture_defines.h"
//api-ms-win-core-synch-l1-2-1.dll
class FfmpegMediaProcess{
public:
	virtual ~FfmpegMediaProcess();
	int init(std::string inputFileName);

	void setVideoCallBack(rtc::VideoSinkInterface<webrtc::VideoFrame>* dataCallBack);

	static void process_thread(FfmpegMediaProcess *ffmp);
	void process();
	static FfmpegMediaProcess* Create(); 
	static FfmpegMediaProcess* ffmp;

	void start();
	void stop();
	#if 0
	int ffmpeg_init(const char *inputFileName);
	int ffmpeg_av_read_frame(AVPacket *packet);
	
	int ffmpeg_get_buffer_fromCodec(AVPacket *packet, AVFrame *frame);
	#endif
	rtc::VideoSinkInterface<webrtc::VideoFrame>* _dataCallBack;

	

	AVCodecContext* dec_ctx;
	AVCodec* dec;
	AVFormatContext* fmt_ctx;
	
	int video_stream_index;

	bool running;
	std::thread threadObj;
};
#endif
