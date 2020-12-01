#ifndef API_FFMPEG_H_
#define API_FFMPEG_H_


#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//#include "libavfilter/avfiltergraph.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#if defined(WEBRTC_LINUX)
#define DLLEXPORT
#elif defined(WEBRTC_WIN)
#define DLLEXPORT __declspec(dllexport)
#endif

DLLEXPORT int ffmpeg_init(const char *inputFileName);
DLLEXPORT int ffmpeg_av_read_frame(AVPacket *packet);

DLLEXPORT int ffmpeg_get_buffer_fromCodec(AVPacket *packet, AVFrame *frame);

#endif

