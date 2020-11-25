#include "ffmpeg-process.h"


#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "common_types.h"  // NOLINT(build/include)
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_capture/video_capture_config.h"
#include "modules/video_capture/video_capture_impl.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/trace_event.h"
#include "third_party/libyuv/include/libyuv.h"
#include <iostream>

#if defined(WEBRTC_WIN)
#include <windows.h>
#endif

using namespace  webrtc;

int FfmpegMediaProcess::init(std::string inputFileName){
	video_stream_index = ffmpeg_init(inputFileName.c_str());
    return 0;
}

FfmpegMediaProcess* FfmpegMediaProcess::Create(){
	return new FfmpegMediaProcess();
}

FfmpegMediaProcess::~FfmpegMediaProcess() {
  //Destroy();
}

void FfmpegMediaProcess::setVideoCallBack(rtc::VideoSinkInterface<webrtc::VideoFrame>* dataCallBack){
	_dataCallBack = dataCallBack;
}


void FfmpegMediaProcess::start(){
	running = true;
	threadObj = std::thread(std::ref(FfmpegMediaProcess::process_thread), this);
	threadObj.detach();
}

void FfmpegMediaProcess::stop(){
	running = false;
}

void FfmpegMediaProcess::process_thread(FfmpegMediaProcess *ffmp){
	ffmp->process();
}

void FfmpegMediaProcess::process(){
	AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int got_frame;
	int ret;
	
    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }
	
	while(running){
		if ((ret = ffmpeg_av_read_frame(&packet)) < 0){
			#if defined(WEBRTC_LINUX)
			usleep(1000);
			#elif defined(WEBRTC_WIN)
			Sleep(1);
			#endif
			continue;
		}
		
		if (packet.stream_index == video_stream_index) {
			got_frame = ffmpeg_get_buffer_fromCodec(&packet, frame);
			if (got_frame == 1) {
				/* Unreference all the buffers referenced by frame and reset the frame fields. */
				rtc::scoped_refptr<I420Buffer> buffer = I420Buffer::Create(
 					 frame->width, frame->height, frame->width, (frame->width + 1) / 2, (frame->width + 1) / 2);
				/*
				const int conversionResult = libyuv::ConvertToI420(
			      videoFrame, videoFrameLength, buffer.get()->MutableDataY(),
			      buffer.get()->StrideY(), buffer.get()->MutableDataU(),
			      buffer.get()->StrideU(), buffer.get()->MutableDataV(),
			      buffer.get()->StrideV(), 0, 0,  // No Cropping
			      width, height, target_width, target_height, rotation_mode,
			      ConvertVideoType(frameInfo.videoType));
				*/
				std::cout << "frame: " << frame->width << "," << frame->height << std::endl;
				for(int i = 0; i < frame->height; i++){
					//memcpy(buffer.get()->MutableDataY()+ i*frame->width, 
					memcpy(buffer.get()->MutableDataY()+ i*buffer.get()->StrideY(), 
						frame->data[0]+i*frame->linesize[0], 
						frame->width);
				}
				for(int i = 0; i < frame->height/2; i++){
					//memcpy(buffer.get()->MutableDataU()+ i*(frame->width+1)/2,
					memcpy(buffer.get()->MutableDataU()+ buffer.get()->StrideU(),
						frame->data[1]+i*frame->linesize[1], 
						(frame->width+1)/2);
				}
				for(int i = 0; i < frame->height/2; i++){
					//memcpy(buffer.get()->MutableDataV()+ i*(frame->width+1)/2,
					memcpy(buffer.get()->MutableDataV()+ i*buffer.get()->StrideV(),
						frame->data[2]+i*frame->linesize[2],
						(frame->width+1)/2);
				}
				
				VideoFrame captureFrame =
					  VideoFrame::Builder()
						  .set_video_frame_buffer(buffer)
						  .set_timestamp_rtp(0)
						  .set_timestamp_ms(rtc::TimeMillis())
						  .set_rotation(/*!apply_rotation ? _rotateFrame : */kVideoRotation_0)
						  .build();
				captureFrame.set_ntp_time_ms(0);
				if(_dataCallBack){
					_dataCallBack->OnFrame(captureFrame);
				}
			}
			av_frame_unref(frame);
		}
		#if defined(WEBRTC_LINUX)
		usleep(30*1000);
		#elif defined(WEBRTC_WIN)
		Sleep(30);
		#endif
		av_packet_unref(&packet);
	}

}


