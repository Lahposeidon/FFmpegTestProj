#include "StdAfx.h"
#include "AVDecode.h"

CAVDecode::CAVDecode(void):
	m_nVideo_Stream_Idx(-1), m_nAudio_Stream_Idx(-1),
	m_nVideo_Frame_Count(0), m_nAudio_Frame_Count(0)
{
}


CAVDecode::~CAVDecode(void)
{
	if (m_pVideoCodecCtx)
		avcodec_close(m_pVideoCodecCtx);

	if (m_pAudioCodecCtx)
		avcodec_close(m_pAudioCodecCtx);

	if (m_ifmt_ctx) 
	{
		/* Close the output file. */
		avio_close(m_ifmt_ctx->pb);
		/* free the stream */
		avformat_free_context(m_ifmt_ctx);
	}
}

int CAVDecode::OpenInputFile( const char *filename )
{
	int ret;
	m_ifmt_ctx = NULL;
	m_pAudioCodecCtx = NULL;
	m_pVideoCodecCtx = NULL;
	if ((ret = avformat_open_input(&m_ifmt_ctx, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		return ret;
	}
	if ((ret = avformat_find_stream_info(m_ifmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}

	int video_dst_linesize[4];
	int video_dst_bufsize;
	uint8_t *video_dst_data[4] = {NULL};
	
	if (OpenCodecContext(&m_nVideo_Stream_Idx, m_ifmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0)
	{
		m_pVideo_Stream = m_ifmt_ctx->streams[m_nVideo_Stream_Idx];
		m_pVideoCodecCtx = m_pVideo_Stream->codec;

		/* allocate image where the decoded image will be put */
		ret = av_image_alloc(video_dst_data, video_dst_linesize,
			m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
			m_pVideoCodecCtx->pix_fmt, 1);
		if (ret < 0) {
			fprintf(stderr, "Could not allocate raw video buffer\n");
		}
		video_dst_bufsize = ret;
	}
	if (OpenCodecContext(&m_nAudio_Stream_Idx, m_ifmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
		m_pAudio_Stream = m_ifmt_ctx->streams[m_nAudio_Stream_Idx];
		m_pAudioCodecCtx = m_pAudio_Stream->codec;
	}

	/* dump input information to stderr */
	//av_dump_format(m_ifmt_ctx, 0, filename, 0);
	int64_t lltime = av_gettime_relative();

	if (!m_pAudio_Stream && !m_pVideo_Stream) {
		fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
		ret = 1;
	}
	return 0;
}

int CAVDecode::OpenCodecContext(int *stream_idx,	AVFormatContext *fmt_ctx, enum AVMediaType type)
{
	int ret;
	AVStream *st;
	AVCodecContext *dec_ctx = NULL;
	AVCodec *dec = NULL;
	AVDictionary *opts = NULL;
	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);

	*stream_idx = ret;
	st = fmt_ctx->streams[*stream_idx];
	/* find decoder for the stream */
	dec_ctx = st->codec;

#if defined(MAX_THREAD_COUNT) && (MAX_THREAD_COUNT > 1)
	dec_ctx->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
	{
		DWORD_PTR process;
		DWORD_PTR system;
		int i_cpu = 0;

		if (GetProcessAffinityMask( GetCurrentProcess(), &process, &system) == 0)
			i_cpu = 1;
		else {
			while (system)
			{
				i_cpu += system & 1;
				system = system >> 1;
			}
		}

		dec_ctx->thread_count = FFMIN(MAX_THREAD_COUNT, i_cpu);
#ifdef WIN32
		DebugAndLogPrintA("%s: ALLOWING %d/%d THREAD(S) FOR DECODING", av_get_media_type_string(type), dec_ctx->thread_count, i_cpu);
#else
		fprintf(stderr, "%s: ALLOWING %d/%d THREAD(S) FOR DECODING\n", av_get_media_type_string(type), dec_ctx->thread_count, i_cpu);
#endif
	}
#endif

	dec = avcodec_find_decoder(dec_ctx->codec_id);
	if (!dec) {
		fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
		return AVERROR(EINVAL);
	}
	/* Init the decoders, with or without reference counting */
	if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
		fprintf(stderr, "Failed to open %s codec\n", av_get_media_type_string(type));
		return ret;
	}

	return 0;
}

int CAVDecode::DecodePacket(int *got_frame, AVPacket *pkt, int cached )
{
	AVFrame* frame = av_frame_alloc();
    int ret = 0;
    int decoded = pkt->size;
    if (pkt->stream_index == m_nVideo_Stream_Idx) {
        /* decode video frame */
        ret = avcodec_decode_video2(m_pVideoCodecCtx, frame, got_frame, pkt);
        if (ret < 0) {
#ifdef WIN32
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			DebugAndLogPrintA("Error decoding video frame : %s(%d)", buf, ret);
#else
            fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
#endif
            return ret;
        }
        if (*got_frame) {
			int video_dst_linesize[4];
			uint8_t *video_dst_data[4] = {NULL};
#ifdef WIN32
			char buf[AV_TS_MAX_STRING_SIZE];
			ZeroMemory(buf, AV_TS_MAX_STRING_SIZE);

			DebugAndLogPrintA("video_frame%s n:%d coded_n:%d pts:%s",
				cached ? "(cached)" : "",
				m_nVideo_Frame_Count++, frame->coded_picture_number,
				av_ts_make_time_string(buf, frame->pts, &m_pVideoCodecCtx->time_base));
#else
            fprintf(stderr, "video_frame%s n:%d coded_n:%d pts:%s\n",
				cached ? "(cached)" : "",
				m_nVideo_Frame_Count++, frame->coded_picture_number,
				av_ts2timestr(frame->pts, &m_pVideoCodecCtx->time_base));
#endif

            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(video_dst_data, video_dst_linesize,
				(const uint8_t **)(frame->data), frame->linesize,
				m_pVideoCodecCtx->pix_fmt, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);
            
			/* write to rawvideo file */
            //fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
        }
    } else if (pkt->stream_index == m_nAudio_Stream_Idx) {
        /* decode audio frame */
        ret = avcodec_decode_audio4(m_pAudioCodecCtx, frame, got_frame, pkt);
        if (ret < 0) {
#ifdef WIN32
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			DebugAndLogPrintA("Error decoding audio frame : %s(%d)", buf, ret);
#else
            fprintf(stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
#endif
            return ret;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, pkt->size);
        if (*got_frame) {
            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);
#ifdef WIN32
			char buf[AV_TS_MAX_STRING_SIZE];
			ZeroMemory(buf, AV_TS_MAX_STRING_SIZE);

            DebugAndLogPrintA("audio_frame%s n:%d nb_samples:%d pts:%s",
				cached ? "(cached)" : "",
				m_nAudio_Frame_Count++, frame->nb_samples,
				av_ts_make_time_string(buf, frame->pts, &m_pVideoCodecCtx->time_base));
#else
			fprintf(stderr, "audio_frame%s n:%d nb_samples:%d pts:%s\n",
				cached ? "(cached)" : "",
				m_nAudio_Frame_Count++, frame->nb_samples,
				av_ts2timestr(frame->pts, &m_pAudioCodecCtx->time_base));
#endif

            /* Write the raw audio data samples of the first plane. This works
             * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
             * most audio decoders output planar audio, which uses a separate
             * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
             * In other words, this code will write only the first audio channel
             * in these cases.
             * You should use libswresample or libavfilter to convert the frame
             * to packed data. */
            //fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
        }
    }

	av_frame_free(&frame);
    return decoded;
}