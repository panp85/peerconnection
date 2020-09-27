/*============================================================================= 
#     FileName: filter_video.c 
#         Desc: an example of ffmpeg fileter
#       Author: licaibiao 
#   LastChange: 2017-03-16  
=============================================================================*/ 
#define _XOPEN_SOURCE 600 /* for usleep */
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"

#define SAVE_FILE

const char *filter_descr = "scale=iw*2:ih*2";
static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;
static int64_t last_pts = AV_NOPTS_VALUE;

static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream  判断流是否正常 */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    dec_ctx = fmt_ctx->streams[video_stream_index]->codec;
    av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0); /* refcounted_frames 帧引用计数 */

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");     /* 输入buffer filter */
    AVFilter *buffersink = avfilter_get_by_name("buffersink"); /* 输出buffer filter */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;   /* 时间基数 */

#ifndef SAVE_FILE
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
#else
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
#endif

    filter_graph = avfilter_graph_alloc();                     /* 创建graph  */
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);
	av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
    /* 创建并向FilterGraph中添加一个Filter */
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);           
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);          
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

     /* Set a binary option to an integer list. */
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);   
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    /* Add a graph described by a string to a graph */
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)    
        goto end;

    /* Check validity and configure all the links and formats in the graph */
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)   
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

#ifndef SAVE_FILE
static void display_frame(const AVFrame *frame, AVRational time_base)
{
    int x, y;
    uint8_t *p0, *p;
    int64_t delay;

    if (frame->pts != AV_NOPTS_VALUE) {
        if (last_pts != AV_NOPTS_VALUE) {
            /* sleep roughly the right amount of time;
             * usleep is in microseconds, just like AV_TIME_BASE. */
             /* 计算 pts 是用来把时间戳从一个时基调整到另外一个时基时候用的函数 */
            delay = av_rescale_q(frame->pts - last_pts,
                                 time_base, AV_TIME_BASE_Q);
            if (delay > 0 && delay < 1000000)
                usleep(delay);
        }
        last_pts = frame->pts;
    }

    /* Trivial ASCII grayscale display. */
    p0 = frame->data[0];
    puts("\033c");
    for (y = 0; y < frame->height; y++) {
        p = p0;
        for (x = 0; x < frame->width; x++)
            putchar(" .-+#"[*(p++) / 52]);
        putchar('\n');
        p0 += frame->linesize[0];
    }
    fflush(stdout);
}
#else
FILE * file_fd;
static void write_frame(const AVFrame *frame)
{
	static int num = 0;
	
	static int printf_flag = 0;
	if(num++ < 50){
		return;
	}
	if(!printf_flag){
		printf_flag = 1;
    		printf("frame widht=%d,frame height=%d\n",frame->width,frame->height);
    	
		if(frame->format==AV_PIX_FMT_YUV420P){
       		printf("format is yuv420p\n");
   	 	}
		else{
       		printf("formet is = %d \n",frame->format);
    	}
	
	}

    fwrite(frame->data[0],1,frame->width*frame->height,file_fd);
    fwrite(frame->data[2],1,frame->width/2*frame->height/2,file_fd);
    fwrite(frame->data[1],1,frame->width/2*frame->height/2,file_fd);
}

#endif

int main(int argc, char **argv)
{
    int ret;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int got_frame;

#ifdef SAVE_FILE
    file_fd = fopen("test.yuv","wb+");
#endif

    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        exit(1);
    }

    av_register_all();
    avfilter_register_all();

    if ((ret = open_input_file(argv[1])) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;

        if (packet.stream_index == video_stream_index) {
            got_frame = 0;
            ret = avcodec_decode_video2(dec_ctx, frame, &got_frame, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
                break;
            }

            if (got_frame) {
                frame->pts = av_frame_get_best_effort_timestamp(frame);    /* pts: Presentation Time Stamp */

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered frames from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;
#ifndef SAVE_FILE
                    display_frame(filt_frame, buffersink_ctx->inputs[0]->time_base);
#else
                    write_frame(filt_frame);
#endif
                    av_frame_unref(filt_frame);
                }
                /* Unreference all the buffers referenced by frame and reset the frame fields. */
                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }
end:
    avfilter_graph_free(&filter_graph);
    avcodec_close(dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }
#ifdef SAVE_FILE
    fclose(file_fd);
#endif
    exit(0);
}
