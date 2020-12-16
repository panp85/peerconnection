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
//#pragma comment(lib, "ffmpeg-api.lib")
#endif

using namespace  webrtc;

int FfmpegMediaProcess::init(std::string inputFileName){
	file_name = inputFileName;
	video_stream_index = ffmpeg_init(inputFileName.c_str());
	file_fd = fopen("TEST.yuv","wb+");
	
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
	av_log(NULL, AV_LOG_ERROR, "start\n");
	threadObj = std::thread(std::ref(FfmpegMediaProcess::process_thread), this);
	threadObj.detach();
}

void FfmpegMediaProcess::stop(){
	running = false;
}

void FfmpegMediaProcess::process_thread(FfmpegMediaProcess *ffmp){
	ffmp->process();
}


void FfmpegMediaProcess::write_file(unsigned char *buf, int len){
	if(file_fd)
		fwrite(buf, 1, len, file_fd);
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
			if(ret == AVERROR_EOF){
				video_stream_index = ffmpeg_init(file_name.c_str());
			}
			#if defined(WEBRTC_LINUX)
			usleep(1000);
			#elif defined(WEBRTC_WIN)
			Sleep(1);
			#endif
			continue;
		}
		
		if (packet.stream_index == video_stream_index) {
			got_frame = ffmpeg_get_buffer_fromCodec(&packet, frame);
			if(got_frame == 2){
				video_stream_index = ffmpeg_init(file_name.c_str());
			}
			else if (got_frame == 1) {
				/* Unreference all the buffers referenced by frame and reset the frame fields. */
				rtc::scoped_refptr<I420Buffer> buffer = I420Buffer::Create(
 					 frame->width, frame->height, frame->width, frame->width / 2, frame->width / 2);
				/*
				const int conversionResult = libyuv::ConvertToI420(
			      videoFrame, videoFrameLength, buffer.get()->MutableDataY(),
			      buffer.get()->StrideY(), buffer.get()->MutableDataU(),
			      buffer.get()->StrideU(), buffer.get()->MutableDataV(),
			      buffer.get()->StrideV(), 0, 0,  // No Cropping
			      width, height, target_width, target_height, rotation_mode,
			      ConvertVideoType(frameInfo.videoType));
				*/
				std::cout << "frame: " << frame->width << "," << frame->height 
					<< "; linesize: " <<frame->linesize[0] <<", "<< frame->linesize[1] << ", "<< frame->linesize[2] << std::endl;
				for(int i = 0; i < frame->height; i++){
					//memcpy(buffer.get()->MutableDataY()+ i*frame->width, 
					memcpy(buffer.get()->MutableDataY()+ i*buffer.get()->StrideY(), 
						frame->data[0]+i*frame->linesize[0], 
						frame->width);
					write_file(frame->data[0]+i*frame->linesize[0], frame->width);
				}
				for(int i = 0; i < frame->height/2; i++){
					//memcpy(buffer.get()->MutableDataU()+ i*(frame->width+1)/2,
					memcpy(buffer.get()->MutableDataU()+ i*buffer.get()->StrideU(),
						frame->data[1]+i*frame->linesize[1], 
						frame->width/2);
					write_file(frame->data[1]+i*frame->linesize[1], frame->width/2);
				}
				for(int i = 0; i < frame->height/2; i++){
					//memcpy(buffer.get()->MutableDataV()+ i*(frame->width+1)/2,
					memcpy(buffer.get()->MutableDataV()+ i*buffer.get()->StrideV(),
						frame->data[2]+i*frame->linesize[2],
						frame->width/2);
					write_file(frame->data[2]+i*frame->linesize[2], frame->width/2);
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
			//av_frame_unref(frame);
		}
		#if defined(WEBRTC_LINUX)
		usleep(5*1000);
		#elif defined(WEBRTC_WIN)
		Sleep(5);
		#endif
		//av_packet_unref(&packet);
	}

}

#if 0
int FfmpegMediaProcess::ffmpeg_init(const char* inputFileName) {
  int ret;
  // AVFormatContext *fmt_ctx = *fmt_ctx_p;
  // printf ("ppt, in ffmpeg_init, go in.\n");
  // av_register_all();
  // avfilter_register_all();

  av_log(NULL, AV_LOG_ERROR, "inputFileName: %s\n", inputFileName);
  std::cout << "inputFileName: " << inputFileName << std::endl;

  if ((ret = avformat_open_input(&fmt_ctx, inputFileName, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file, ret: %d\n", ret);
	std::cout << "Cannot open input file" << std::endl;
    return ret;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  /* select the video stream	判断流是否正常 */
  int video_index =
      av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
  if (video_index < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Cannot find a video stream in the input file\n");
    return video_index;
  }
  // video_stream_index = ret;
  dec_ctx = fmt_ctx->streams[video_index]->codec;
  av_opt_set_int(dec_ctx, "refcounted_frames", 1,
                 0); /* refcounted_frames 帧引用计数 */

  /* init the video decoder */
  if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
    return ret;
  }

  return video_index;
}

int FfmpegMediaProcess::ffmpeg_av_read_frame(AVPacket* packet) {
  int ret;
  if ((ret = av_read_frame(fmt_ctx, packet)) < 0)
    return ret;
  return 0;
}

int FfmpegMediaProcess::ffmpeg_get_buffer_fromCodec(AVPacket* packet, AVFrame* frame) {
  //int got_frame = 0;
  int ret = avcodec_send_packet(dec_ctx, packet);
  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    return 0;
  }
  //if (ret >= 0)
  //  packet->size = 0;
  ret = avcodec_receive_frame(dec_ctx, frame);
  if (ret >= 0)
    return 1;
  
  return 0;
}
#endif
