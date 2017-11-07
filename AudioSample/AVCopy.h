#pragma once
#ifndef AVCOPY_H_
#define AVCOPY_H_

#include "FFmpeg.h"

extern "C" {
	#include "libavutil/common.h"
	#include "libavutil/mem.h"
	#include "libavutil/error.h"
	#include "libavutil/rational.h"
};

class CAVCopy
{
public:
	CAVCopy(const char *i_file, const char *o_file, int &error_code);
	~CAVCopy(void);

	int copy();
	int merge(const char *i_file);
	int end();
protected:
	AVFormatContext *m_input_fmt_ctx;
	AVFormatContext	*m_output_fmt_ctx;
	AVStream *m_OutputStream;

	static int start( AVStream *dst, AVFormatContext *oc, AVStream *est);
	inline char *const get_error_text(const int error)
	{
		static char error_buffer[255];
		av_strerror(error, error_buffer, sizeof(error_buffer));
		return error_buffer;
	}

	int64_t m_nlast_pts;
	int64_t m_nlast_dts;
};

#endif /* AVCOPY_H_ */