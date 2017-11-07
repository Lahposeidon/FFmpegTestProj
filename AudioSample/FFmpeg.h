#pragma once

#if !defined(__STDC_LIMIT_MACROS)
#  define __STDC_LIMIT_MACROS
#endif

#if !defined(__STDC_CONSTANT_MACROS)
#  define __STDC_CONSTANT_MACROS
#endif

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/frame.h>
	#include <libswresample/swresample.h>
	#include <libavutil/avassert.h>
	#include <libavutil/audio_fifo.h>
	#include <libavutil/opt.h>
	#include <libavutil/avstring.h>
	#include <libavutil/time.h>
}