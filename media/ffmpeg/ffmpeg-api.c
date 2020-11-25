#include "ffmpeg-api.h"

AVCodecContext *dec_ctx;	
AVCodec *dec;
AVFormatContext *fmt_ctx;

int ffmpeg_init(const char *inputFileName){
	int ret;
	//AVFormatContext *fmt_ctx = *fmt_ctx_p;
	
	av_register_all();
	avfilter_register_all();
	
	av_log(NULL, AV_LOG_ERROR, "inputFileName: %s\n", inputFileName);

	if ((ret = avformat_open_input(&fmt_ctx, inputFileName, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file, ret: %d\n", ret);
		return ret;
	}

	if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}

	/* select the video stream	判断流是否正常 */
	int video_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	if (video_index < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
		return video_index;
	}
	//video_stream_index = ret;
	dec_ctx = fmt_ctx->streams[video_index]->codec;
	av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0); /* refcounted_frames 帧引用计数 */

	/* init the video decoder */
	if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
		return ret;
	}

	return video_index;
}


int ffmpeg_av_read_frame(AVPacket* packet){
	int ret;
	if ((ret = av_read_frame(fmt_ctx, packet)) < 0)
		return ret;
	return 0;
}


int ffmpeg_get_buffer_fromCodec(AVPacket *packet, AVFrame *frame){
	int got_frame = 0;
	int ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, packet);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
		return ret;
	}
	return got_frame;
}
