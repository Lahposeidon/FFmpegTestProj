#pragma once
#ifndef AVENCODE_H_
#define AVENCODE_H_

#include "FFmpeg.h"

class CAVEncode
{
public:
	CAVEncode(void);
	virtual ~CAVEncode(void);

private:
	AVFormatContext *m_ofmt_ctx;;
	AVCodecContext *m_pVideoCodecCtx;
	AVCodecContext *m_pAudioCodecCtx;
	AVStream *m_pVideoStream;
	AVStream *m_pAudioStream;

	AVCodec *m_pAudioCodec;
	AVCodec *m_pVideoCodec;
	double m_dAudioTime;
	double m_dVideoTime;

	AVFrame *m_pVideoFrame;
	AVPicture m_SrcPicture;
	AVPicture m_DstPicture;
	int m_nFrameCount;

	uint8_t	**m_pSrcSamplesData;
	uint8_t	**m_pDstSamplesData;
	AVFrame *m_pAudioFrame;

private:
	void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
	void OpenVideo(AVFormatContext *oc, AVCodec *codec, AVStream *st);
	void OpenAudio(AVFormatContext *oc, AVCodec *codec, AVStream *st);
	AVStream* AddStream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, enum AVMediaType type);
	void CloseVideo(AVFormatContext *oc, AVStream *st);
	void CloseAudio(AVFormatContext *oc, AVStream *st);
	void FillYuvImage(AVPicture *pict, int frame_index, int width, int height);
public:
	void End();
	int OpenOutputFile(const char *filename);
	AVFormatContext* getFormatContext(){ return m_ofmt_ctx;}
};

#endif /* AVENCODE_HPP_ */