#include "StdAfx.h"
#include "AVCopy.h"
#include <stdexcept>

CAVCopy::CAVCopy(const char *i_file, const char *o_file, int &error_code) 
	: m_nlast_pts(0), m_nlast_dts(0)
{
	int error = 0;
	m_output_fmt_ctx = m_input_fmt_ctx = NULL; 

	fprintf( stderr, " => '%s'\n", i_file);
	if ((error = avformat_open_input( &m_input_fmt_ctx, i_file, NULL, 0)) < 0 ||
		(m_input_fmt_ctx->nb_streams == 0))
	{
		fprintf(stderr, "Could not open input file '%s' %(error '%s')\n",
			i_file, get_error_text(error));
		m_input_fmt_ctx = NULL;
		error_code = error;
		return;
	}

	//av_dump_format(m_input_fmt_ctx, 0, i_file, 0);

	if ((error = avformat_find_stream_info(m_input_fmt_ctx, NULL)) < 0) {
		fprintf(stderr, "Could not open find stream info (error '%s')\n",
			get_error_text(error));
		avformat_close_input(&m_input_fmt_ctx);
		error_code = error;
		return;
	}

	for (size_t n = 0; n < m_input_fmt_ctx->nb_streams; n++)
	{
		AVStream *pInputStream = m_input_fmt_ctx->streams[n];
		if (pInputStream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if (!(m_output_fmt_ctx = avformat_alloc_context()))
			{
				fprintf(stderr, "avformat_alloc_context error \n");
				avformat_close_input(&m_input_fmt_ctx);
				error_code = -1;
				return;
			}
			else {
				m_output_fmt_ctx->oformat = av_guess_format( NULL, o_file, NULL);
				av_strlcpy( m_output_fmt_ctx->filename, o_file, sizeof(m_output_fmt_ctx->filename));

				if (!(m_output_fmt_ctx->oformat->flags & AVFMT_NOFILE) &&
					(avio_open( &m_output_fmt_ctx->pb, o_file, AVIO_FLAG_WRITE) < 0))
				{
					throw std::runtime_error( "avio_open");
				}

#define DEF_PRELOAD             0.5
#define DEF_MAX_DELAY           0.7
				m_output_fmt_ctx->audio_preload = (int)(DEF_PRELOAD * AV_TIME_BASE);
				m_output_fmt_ctx->max_delay = (int)(DEF_MAX_DELAY * AV_TIME_BASE);
				m_output_fmt_ctx->flags |= AVFMT_FLAG_NONBLOCK;
			}

			if (!(m_OutputStream = avformat_new_stream( m_output_fmt_ctx, NULL)))
				throw std::runtime_error( "av_new_stream");
			else {
				if (this->start( pInputStream, m_output_fmt_ctx, m_OutputStream) < 0)
					throw std::runtime_error( "start");
			}

			error_code = 0;
		}
	}
}

CAVCopy::~CAVCopy(void)
{
	if (m_OutputStream)
	{
		av_free( m_OutputStream->codec->extradata);
		av_free( m_OutputStream);
	}

	if (m_input_fmt_ctx)
		avformat_close_input(&m_input_fmt_ctx);

	if (m_output_fmt_ctx)
		avio_close(m_output_fmt_ctx->pb);
}

int CAVCopy::copy()
{
	AVPacket pkt;
    int r;

_gW:if ((r = av_read_frame( m_input_fmt_ctx, &pkt)) == AVERROR(EAGAIN))
        goto _gW;
    else {
        if (r == 0)
        {
            AVPacket o_pkt;
            AVStream *ist = m_input_fmt_ctx->streams[pkt.stream_index];

            if (ist->codec->codec_type != AVMEDIA_TYPE_AUDIO)
                fprintf( stderr, "%d", ist->codec->codec_type);
            else {
                av_init_packet( &o_pkt);

                /* this->pts = this->next_pts;
                this->next_pts +=
                    ((int64_t)AV_TIME_BASE * ist->codec->frame_size) / ist->codec->sample_rate;
                 */

                o_pkt.stream_index = pkt.stream_index;
                if (pkt.pts == (int64_t)AV_NOPTS_VALUE)
					o_pkt.pts = AV_NOPTS_VALUE;
                else
					o_pkt.pts = av_rescale_q( pkt.pts, ist->time_base, m_OutputStream->time_base);

                if (pkt.dts == (int64_t)AV_NOPTS_VALUE)
					o_pkt.dts = AV_NOPTS_VALUE;
                else
					o_pkt.dts = av_rescale_q( pkt.dts, ist->time_base, m_OutputStream->time_base);

                o_pkt.duration = static_cast<int>(av_rescale_q( pkt.duration, ist->time_base, m_OutputStream->time_base));
                o_pkt.flags = pkt.flags;

                if (av_parser_change( ist->parser, m_OutputStream->codec, &o_pkt.data, &o_pkt.size,
                            pkt.data, pkt.size, o_pkt.flags & AV_PKT_FLAG_KEY))
					av_free_packet(&pkt);

                if (av_interleaved_write_frame( m_output_fmt_ctx, &o_pkt) < 0)
                    av_free_packet(&o_pkt);
                else {
                    av_free_packet(&o_pkt);
                    goto _gW;
                } return -1;
            } goto _gW;
        }
    } return 0;
}

int CAVCopy::start(AVStream *dst, AVFormatContext *oc, AVStream *est)
{
	AVCodecContext *dec = dst->codec,
		*enc = est->codec;

	est->disposition = est->disposition;
	enc->bits_per_raw_sample = dec->bits_per_raw_sample;
	enc->chroma_sample_location = dec->chroma_sample_location;
	{
		uint64_t extra_size = (uint64_t)dec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE;
		fprintf(stderr, "[%lu] - [%d]\n", extra_size, INT_MAX);
		if (extra_size > INT_MAX)
			return AVERROR(EINVAL);
		else {
			enc->codec_id = dec->codec_id;
			enc->codec_type = dec->codec_type;
			if (!enc->codec_tag)
			{
				if (!oc->oformat->codec_tag ||
					(av_codec_get_id( oc->oformat->codec_tag, dec->codec_tag) == enc->codec_id) ||
					(av_codec_get_tag( oc->oformat->codec_tag, dec->codec_id) <= 0))
					enc->codec_tag = dec->codec_tag;
			}

			enc->bit_rate = dec->bit_rate;
			enc->rc_max_rate = dec->rc_max_rate;
			enc->rc_buffer_size = dec->rc_buffer_size;
			if (!(enc->extradata = (uint8_t *)av_mallocz( static_cast<size_t>(extra_size))))
				return AVERROR(ENOMEM);
			else {
				memcpy( enc->extradata, dec->extradata, dec->extradata_size);
				enc->extradata_size = dec->extradata_size;
			}

			enc->time_base = dst->time_base;
			av_reduce( &enc->time_base.num, &enc->time_base.den,
				enc->time_base.num, enc->time_base.den, INT_MAX);

			enc->channel_layout = dec->channel_layout;
			enc->sample_rate = dec->sample_rate;
			enc->channels = dec->channels;
			enc->frame_size = dec->frame_size;
			enc->block_align = dec->block_align;
		}
	}

#if 1
	if ((strrchr(oc->filename, '\\')) == NULL)
		av_dict_set( &oc->metadata, "title", oc->filename, AV_DICT_APPEND);
	else
		av_dict_set( &oc->metadata, "title", strrchr( oc->filename, '\\') + 1, AV_DICT_APPEND);

	av_dict_set( &oc->metadata, "artist", "", AV_DICT_APPEND);
	av_dict_set( &oc->metadata, "album", "PODCAST", AV_DICT_APPEND);
	av_dict_set( &oc->metadata, "comment", "Build for Double U", AV_DICT_APPEND);
	av_dict_set( &oc->metadata, "genre", "podcast", AV_DICT_APPEND);
#endif
	return avformat_write_header(oc, &oc->metadata);
}

int CAVCopy::merge(const char *i_file)
{
	    AVFormatContext  *i_fmt_ctx = NULL;
        if (avformat_open_input(&i_fmt_ctx, i_file, NULL, NULL)!=0)
        {
            fprintf(stderr, "could not open input file\n");
            return -1;
        }

        if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
        {
            fprintf(stderr, "could not find stream info\n");
            return -1;
        }
        
		//av_dump_format(i_fmt_ctx, 0, i_file, 0);

        AVStream *i_stream = NULL;
        for (unsigned s=0; s<i_fmt_ctx->nb_streams; s++)
		{
            if (i_fmt_ctx->streams[s]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                i_stream = i_fmt_ctx->streams[s];
                break;
			}
		}

        if (i_stream == NULL)
        {
            fprintf(stderr, "didn't find any audio stream\n");
            return -1;
        }

        int64_t pts=0, dts=0;
        while (1)
        {
            AVPacket i_pkt;
            av_init_packet(&i_pkt);
            i_pkt.size = 0;
            i_pkt.data = NULL;
            if (av_read_frame(i_fmt_ctx, &i_pkt) <0 )
                break;
            /*
             * pts and dts should increase monotonically
             * pts should be >= dts
             */
            i_pkt.flags |= AV_PKT_FLAG_KEY;
            pts = i_pkt.pts;
            i_pkt.pts += m_nlast_pts;
            dts = i_pkt.dts;
            i_pkt.dts += m_nlast_dts;
            i_pkt.stream_index = 0;

            static int num = 1;
            fprintf(stderr, "frame %d\n", num++);
            av_interleaved_write_frame(m_output_fmt_ctx, &i_pkt);
        }
        m_nlast_pts += dts;
        m_nlast_dts += pts;

        avformat_close_input(&i_fmt_ctx);
		return 0;
}

int CAVCopy::end()
{
	return av_write_trailer( m_output_fmt_ctx);
}
