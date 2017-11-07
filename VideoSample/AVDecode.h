#pragma once
#ifndef AVDECODE_H_
#define AVDECODE_H_

#include "FFmpeg.h"

class CAVDecode
{
public:
	CAVDecode(void);
	virtual ~CAVDecode(void);


private:
	AVFormatContext *m_ifmt_ctx;
	AVCodecContext *m_pVideoCodecCtx;
	AVCodecContext *m_pAudioCodecCtx;
	AVStream *m_pVideo_Stream;
	AVStream *m_pAudio_Stream;
	int m_nVideo_Stream_Idx;
	int m_nAudio_Stream_Idx;
	int m_nVideo_Frame_Count;
	int m_nAudio_Frame_Count;
	FilteringContext *m_filter_ctx;

	int OpenCodecContext(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type);
	
public:
	int OpenInputFile(const char *filename);
	AVFormatContext* getFormatContext(){return m_ifmt_ctx;}
	int DecodePacket(int *got_frame, AVPacket *pkt, int cached );
};

#endif /* AVDECODE_HPP_ */