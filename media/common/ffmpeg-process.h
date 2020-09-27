#ifndef TEST_FFMPEG_H_
#define TEST_FFMPEG_H_

#include <unistd.h>
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

class FfmpegMediaProcess{
public:
	virtual ~FfmpegMediaProcess();
	int init(char *inputFileName);

	void setVideoCallBack(rtc::VideoSinkInterface<webrtc::VideoFrame>* dataCallBack);

	static void process_thread(FfmpegMediaProcess *ffmp);
	void process();
	static FfmpegMediaProcess* Create(); 
	static FfmpegMediaProcess* ffmp;

	void start();
	void stop();

	rtc::VideoSinkInterface<webrtc::VideoFrame>* _dataCallBack;

	
	
	int video_stream_index;

	bool running;
	std::thread threadObj;
};
#endif
