#pragma once

#if !defined(__STDC_LIMIT_MACROS)
	#define __STDC_LIMIT_MACROS
#endif

#if !defined(__STDC_CONSTANT_MACROS)
	#define __STDC_CONSTANT_MACROS
#endif

#if !defined(__STDC_FORMAT_MACROS)
	#define __STDC_FORMAT_MACROS
#endif

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/frame.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
	#include <libavutil/avassert.h>
	#include <libavutil/opt.h>
	#include <libavutil/avstring.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/timestamp.h>
	#include <libavutil/error.h>
	#include <libavutil/mathematics.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/time.h>

	static int sws_flags = SWS_BICUBIC;
}

typedef struct _FilteringContext {
	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph *filter_graph;
} FilteringContext;

#define MAX_THREAD_COUNT	8
#define LOG_MAX		256

const unsigned int channels = 2;
const unsigned int sample_rate = 44100;
const unsigned int audio_bit_rate = 128000;
#define DEF_SAMPLE_FMT			AV_SAMPLE_FMT_S16
const unsigned int frame_rate = 30000;
#define STREAM_PIX_FMT			AV_PIX_FMT_YUV420P /* default pix_fmt */
const unsigned int picture_width = 720;
const unsigned int picture_height = 1280;
const unsigned int video_bit_rate = 400000;