#include "ffmpeg-api.h"
#include <stdio.h>

#if (defined _WIN32 || defined _WIN64)


//#pragma comment(lib, "D:/msys64/usr/local/ffmpeg-static/bin/avcodec.lib")
//#pragma comment(lib, "D:/msys64/usr/local/ffmpeg-static/bin/avfilter.lib")
//#pragma comment(lib, "D:/msys64/usr/local/ffmpeg-static/bin/avformat.lib")
//#pragma comment(lib, "D:/msys64/usr/local/ffmpeg-static/bin/avutil.lib")
//#pragma comment(lib, "D:/msys64/usr/local/ffmpeg-static/bin/swresample.lib")
//#pragma comment(lib, "D:/msys64/usr/local/ffmpeg-static/bin/swscale.lib")
#include <windows.h>
int dll_init() {}
#pragma comment(lib, "winmm.lib ")

#endif

AVCodecContext* dec_ctx;
AVCodec* dec;
AVFormatContext* fmt_ctx = NULL;
int video_index = -1;
int fd_log;

void usleep_api(int x){
	#if defined(WEBRTC_LINUX)
		usleep(x*1000);
	#elif (defined _WIN32 || defined _WIN64)
		Sleep(x);
	#endif
}

int64_t SystemTimeMillis() {
	int64_t ticks;
#if defined(__linux__)
	struct timespec ts;
	// TODO(deadbeef): Do we need to handle the case when CLOCK_MONOTONIC is not
	// supported?
	clock_gettime(CLOCK_MONOTONIC, &ts);
 	ticks = 1000000000 * ts.tv_sec + ts.tv_nsec;

#elif (defined _WIN32 || defined _WIN64)
	static volatile LONG last_timegettime = 0;
	static volatile int64_t num_wrap_timegettime = 0;
	volatile LONG* last_timegettime_ptr = &last_timegettime;
	DWORD now = timeGetTime();
	// Atomically update the last gotten time
	DWORD old = InterlockedExchange(last_timegettime_ptr, now);
	if (now < old) {
	  // If now is earlier than old, there may have been a race between threads.
	  // 0x0fffffff ~3.1 days, the code will not take that long to execute
	  // so it must have been a wrap around.
	  if (old > 0xf0000000 && now < 0x0fffffff) {
		num_wrap_timegettime++;
	  }
	}
	ticks = now + (num_wrap_timegettime << 32);
	// TODO(deadbeef): Calculate with nanosecond precision. Otherwise, we're
	// just wasting a multiply and divide when doing Time() on Windows.
	ticks = ticks * 1000000;
	av_log(NULL, AV_LOG_ERROR, "SystemTimeMillis: %lld.\n", ticks);
#else
	av_log(NULL, AV_LOG_ERROR, "Unsupported platform.\n");

#endif
	return ticks/1000000;
  }

int ffmpeg_init(const char* inputFileName) {
  int ret;
  // AVFormatContext *fmt_ctx = *fmt_ctx_p;
  // printf ("ppt, in ffmpeg_init, go in.\n");
  // av_register_all();
  // avfilter_register_all();
  

  av_log(NULL, AV_LOG_ERROR, "inputFileName: %s\n", inputFileName);
  if(fmt_ctx){
  	avformat_close_input(&fmt_ctx);
  }
  #if 0
  else{
  	int refd = 0;
  	fd_log = open("ffmpeg.log",O_RDWR|O_CREAT|O_APPEND,0644);
	
  	fflush(stdout);
    setvbuf(stdout, NULL, _IOLBF, 0);
    refd = dup2(fd_log, fileno(stdout));
	if(refd == -1)
	{
			printf("redirect standard out error:%m\n");
			//exit(-1);
	}
	printf("redirect opened: %d.\n", refd);
  }
  #endif
  if ((ret = avformat_open_input(&fmt_ctx, inputFileName, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file, ret: %d\n", ret);
    return ret;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  /* select the video stream	判断流是否正常 */
  video_index =
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

int ffmpeg_av_read_frame(AVPacket* packet) {
  int ret;
  if(!packet){return -1;}
  //av_packet_unref(packet);
  if ((ret = av_read_frame(fmt_ctx, packet)) < 0)
    return ret;
  return 0;
}

int ffmpeg_get_buffer_fromCodec(AVPacket* packet, AVFrame* frame) {
  //int got_frame = 0;
  if(!frame){return -1;}
  //av_frame_unref(frame);
  int ret = avcodec_send_packet(dec_ctx, packet);
  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    return 0;
  }
  if(ret == AVERROR_EOF){
  	return 2;
  }
  //if (ret >= 0)
  //  packet->size = 0;
  ret = avcodec_receive_frame(dec_ctx, frame);
  
  AVRational tb = fmt_ctx->streams[video_index]->time_base;
  //std::cout << "frame time: " << frame->pts << ", pts:" << frame->pts <<", time_base:" << tb.num << "/" << tb.den << std::endl;
  //av_log(NULL, AV_LOG_INFO, "frame time: %lld, time_base: %d/%d.\n", frame->pts, tb.num, tb.den);
  double pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
  pts = pts *1000.;
  static double first_frame_time = -1.;
  static int64_t first_frame_time_system = -1;
  if(first_frame_time == -1.){
  	first_frame_time = pts;
	first_frame_time_system = SystemTimeMillis();
  }
  else if(pts != NAN){
  	double frame_time_diff = pts - first_frame_time;
	int64_t system_time_diff = SystemTimeMillis() - first_frame_time_system;
    int64_t diff = (int64_t)frame_time_diff - system_time_diff;
	av_log(NULL, AV_LOG_INFO, 
		"pts: %d, frame_time_diff: %d;SystemTimeMillis(): %lld, system_time_diff: %d; diff: %d.\n", 
		(int)pts, (int)frame_time_diff, SystemTimeMillis(), (int)system_time_diff, (int)diff);
	if(diff > 70 && diff < 1000){
		usleep_api(30);
	}
	else if(diff >= 1000 || diff < -2000){
		first_frame_time = pts;
		first_frame_time_system = SystemTimeMillis();
	}
  }
  if (ret >= 0)
    return 1;
  
  return 0;
}
