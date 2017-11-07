#include "StdAfx.h"
#include "AVEncode.h"
#include <stdexcept>

//#include "FFmpeg.h"


CAVEncode::CAVEncode(void)
	: m_ofmt_ctx(NULL),	m_pVideoCodecCtx(NULL),
	m_pAudioCodecCtx(NULL),	m_pVideoFrame(NULL), m_pAudioFrame(NULL),
	m_pVideoStream(NULL), m_pAudioStream(NULL)
{
}

CAVEncode::~CAVEncode(void)
{
	if (m_pVideoCodecCtx)
		avcodec_close(m_pVideoCodecCtx);

	if (m_pAudioCodecCtx)
		avcodec_close(m_pAudioCodecCtx);

	/* Close each codec. */
	if (m_pVideoStream)
		CloseVideo(m_ofmt_ctx, m_pVideoStream);
	if (m_pAudioStream)
		CloseAudio(m_ofmt_ctx, m_pAudioStream);

	if (m_ofmt_ctx) 
	{
		/* Close the output file. */
		avio_close(m_ofmt_ctx->pb);
		/* free the stream */
		avformat_free_context(m_ofmt_ctx);
	}
}


void CAVEncode::CloseVideo(AVFormatContext *oc, AVStream *st)
{
	avcodec_close(st->codec);
	/*if(m_SrcPicture.data[0])
	av_free(m_SrcPicture.data[0]);
	if(m_DstPicture.data[0])
	av_free(m_DstPicture.data[0]);*/
	if(m_pVideoFrame)
		av_frame_free(&m_pVideoFrame);
}

void CAVEncode::CloseAudio(AVFormatContext *oc, AVStream *st)
{
	avcodec_close(st->codec);
	if (m_pDstSamplesData != m_pSrcSamplesData) {
		av_free(m_pDstSamplesData[0]);
		av_free(m_pDstSamplesData);
	}
	av_free(m_pSrcSamplesData[0]);
	av_free(m_pSrcSamplesData);
	av_frame_free(&m_pAudioFrame);
}

void CAVEncode::End()
{
	av_write_trailer(m_ofmt_ctx);
}

void CAVEncode::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
	char buf[AV_TS_MAX_STRING_SIZE] = {0};
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
	printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		av_ts_make_string(buf, pkt->pts), av_ts_make_time_string(buf, pkt->pts, time_base),
		av_ts_make_string(buf, pkt->dts), av_ts_make_time_string(buf, pkt->dts, time_base),
		av_ts_make_string(buf, pkt->duration), av_ts_make_time_string(buf, pkt->duration, time_base),
		//av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
		//av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
		//av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
		pkt->stream_index);
}

int CAVEncode::OpenOutputFile(const char *filename)
{
	int ret;

	/* allocate the output media context */
    avformat_alloc_output_context2(&m_ofmt_ctx, NULL, NULL, filename);
    if (!m_ofmt_ctx) {
        fprintf(stderr, "Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&m_ofmt_ctx, NULL, "mpeg", filename);
    }

    if (!m_ofmt_ctx)
        return -1;

	AVOutputFormat *ofmt = m_ofmt_ctx->oformat;
    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (ofmt->video_codec != AV_CODEC_ID_NONE)
        m_pVideoStream = AddStream(m_ofmt_ctx, &m_pVideoCodec, ofmt->video_codec, AVMEDIA_TYPE_VIDEO);
    if (ofmt->audio_codec != AV_CODEC_ID_NONE)
        m_pAudioStream = AddStream(m_ofmt_ctx, &m_pAudioCodec, ofmt->audio_codec, AVMEDIA_TYPE_AUDIO);

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (m_pVideoStream)
		OpenVideo(m_ofmt_ctx, m_pVideoCodec, m_pVideoStream);
    
	if (m_pAudioStream)
        OpenAudio(m_ofmt_ctx, m_pAudioCodec, m_pAudioStream);

    av_dump_format(m_ofmt_ctx, 0, filename, 1);
    /* open the output file, if needed */
    if (!(m_ofmt_ctx->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }
    /* Write the stream header, if any. */
    ret = avformat_write_header(m_ofmt_ctx, NULL);
    if (ret < 0) {
#ifdef WIN32
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		DebugAndLogPrintA("Error occurred when opening output file: %s(%d)", buf, ret);
#else
		fprintf(stderr, AV_LOG_ERROR, "Error occurred when opening output file: %s\n", av_err2str(ret));
#endif
        return ret;
    }

	return ret;
}

/**************************************************************/
/* audio output */
void CAVEncode::OpenAudio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
	int			src_samples_linesize;
	int			src_nb_samples;

	int			max_dst_nb_samples;
	int			dst_samples_linesize;
	int			dst_samples_size;

	SwrContext *swr_ctx = NULL;
	float t, tincr, tincr2;
    AVCodecContext *c;
    int ret;
    c = st->codec;

    /* allocate and init a re-usable frame */
    AVFrame *audio_frame = av_frame_alloc();
    if (!audio_frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }
    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
#ifdef WIN32
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		DebugAndLogPrintA("Could not open audio codec: %s(%d)", buf, ret);
#else
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
#endif
        exit(1);
    }
    /* init signal generator */
    t     = 0.0f;
    tincr = 2.0f * M_PI * 110.0f / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    tincr2 = 2.0f * M_PI * 110.0f / c->sample_rate / c->sample_rate;
    src_nb_samples = c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
        10000 : c->frame_size;
    ret = av_samples_alloc_array_and_samples(&m_pSrcSamplesData, &src_samples_linesize, c->channels,
                                             src_nb_samples, AV_SAMPLE_FMT_S16, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        exit(1);
    }
    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = src_nb_samples;
    /* create resampler context */
    if (c->sample_fmt != AV_SAMPLE_FMT_S16) {
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            exit(1);
        }
        /* set options */
        av_opt_set_int       (swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);
        /* initialize the resampling context */
        if ((ret = swr_init(swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }
        ret = av_samples_alloc_array_and_samples(&m_pDstSamplesData, &dst_samples_linesize, c->channels,
                                                 max_dst_nb_samples, c->sample_fmt, 0);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate destination samples\n");
            exit(1);
        }
    } else {
        m_pDstSamplesData = m_pSrcSamplesData;
    }
    dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, max_dst_nb_samples,
                                                  c->sample_fmt, 0);
}


/**************************************************************/
/* video output */
void CAVEncode::OpenVideo(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    int ret;
    AVCodecContext *c = st->codec;

    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
#ifdef WIN32
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		DebugAndLogPrintA("Could not open video codec: %s(%d)", buf, ret);
#else
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
#endif
        exit(1);
    }
    /* allocate and init a re-usable frame */
    m_pVideoFrame = av_frame_alloc();
    if (!m_pVideoFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    m_pVideoFrame->format = c->pix_fmt;
    m_pVideoFrame->width = c->width;
    m_pVideoFrame->height = c->height;
    /* Allocate the encoded raw picture. */
    ret = avpicture_alloc(&m_DstPicture, c->pix_fmt, c->width, c->height);
    if (ret < 0) {
#ifdef WIN32
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		DebugAndLogPrintA("Could not allocate picture: %s(%d)", buf, ret);
#else
        fprintf(stderr, "Could not allocate picture: %s\n", av_err2str(ret));
#endif
        exit(1);
    }
    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ret = avpicture_alloc(&m_SrcPicture, AV_PIX_FMT_YUV420P, c->width, c->height);
        if (ret < 0) {
#ifdef WIN32
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			DebugAndLogPrintA("Could not allocate temporary picture: %s(%d)", buf, ret);
#else
			fprintf(stderr, "Could not allocate temporary picture: %s\n",av_err2str(ret));
#endif
            exit(1);
        }
    }
    /* copy data and linesize picture pointers to frame */
    *((AVPicture *)m_pVideoFrame) = m_DstPicture;
}

/* Add an output stream. */
AVStream* CAVEncode::AddStream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, enum AVMediaType type)
{
    AVCodecContext *enc_ctx;
    AVStream *st;
    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        exit(1);
    }
    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
	avcodec_get_context_defaults3(st->codec, *codec);
    enc_ctx = st->codec;
#if defined(MAX_THREAD_COUNT) && (MAX_THREAD_COUNT > 1)
	enc_ctx->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
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

		enc_ctx->thread_count = FFMIN(MAX_THREAD_COUNT, i_cpu);
#ifdef WIN32
		DebugAndLogPrintA("%s: ALLOWING %d/%d THREAD(S) FOR DECODING", av_get_media_type_string(type), enc_ctx->thread_count, i_cpu);
#else
		fprintf(stderr, "%s: ALLOWING %d/%d THREAD(S) FOR DECODING\n", av_get_media_type_string(type), dec_ctx->thread_count, i_cpu);
#endif
	}
#endif

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        enc_ctx->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : DEF_SAMPLE_FMT;
        enc_ctx->bit_rate    = audio_bit_rate;
        enc_ctx->sample_rate = sample_rate;
        enc_ctx->channels    = channels;
        break;
    case AVMEDIA_TYPE_VIDEO:
        enc_ctx->codec_id = codec_id;
        enc_ctx->bit_rate = video_bit_rate;
        /* Resolution must be a multiple of two. */
        enc_ctx->width    = picture_width;
        enc_ctx->height   = picture_height;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        enc_ctx->time_base.den = frame_rate;
        enc_ctx->time_base.num = 1001;
        enc_ctx->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        enc_ctx->pix_fmt       = STREAM_PIX_FMT;
        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B frames */
            enc_ctx->max_b_frames = 2;
        }
        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            enc_ctx->mb_decision = 2;
        }
		/*st->sample_aspect_ratio.num = 1;
		st->sample_aspect_ratio.den = 1;

		c->sample_aspect_ratio.num = 1;
		c->sample_aspect_ratio.den = 1;*/
		/*c->chroma_sample_location = AVCHROMA_LOC_LEFT;
		c->bits_per_raw_sample = 8;*/
		enc_ctx->profile = FF_PROFILE_H264_BASELINE;
		enc_ctx->level = 30;
		break;
    default:
        break;
    }
    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    return st;
}

/* Prepare a dummy image. */
void CAVEncode::FillYuvImage(AVPicture *pict, int frame_index,
	int width, int height)
{
	int x, y, i;
	i = frame_index;
	/* Y */
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
	/* Cb and Cr */
	for (y = 0; y < height / 2; y++) {
		for (x = 0; x < width / 2; x++) {
			pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
			pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
		}
	}
}