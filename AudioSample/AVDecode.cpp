#include "StdAfx.h"
#include "AVDecode.h"


CAVDecode::CAVDecode(void)
{
}

CAVDecode::CAVDecode( const char* file, AVCodecContext *output_codec_context, AVInputFormat *iformat /*= NULL*/ )
{
	if( initialize(file)  == 0)
	{
		init_resampler(getCodecContext(), output_codec_context);
	}
}

CAVDecode::~CAVDecode(void)
{
	swr_free(&m_Resample_ctx);

	if (m_input_codec_ctx)
	{
		avcodec_close(m_input_codec_ctx);
		m_input_codec_ctx = NULL;
	}
	if (m_input_fmt_ctx)
	{
		avformat_close_input(&m_input_fmt_ctx);
		m_input_fmt_ctx = NULL;
	}
}

int CAVDecode::initialize( const char *file, AVInputFormat *iformat /*= NULL*/ )
{
	AVCodec *input_codec;
	int error = 0;
	m_input_fmt_ctx = NULL;
	if ((avformat_open_input( &m_input_fmt_ctx, file, iformat, NULL) < 0))
	{
		fprintf(stderr, "Could not open input file '%s' (error '%s')\n",
			file, get_error_text(error));
		m_input_fmt_ctx = NULL;
		return error;
	}

	/** Get information on the input file (number of streams etc.). */
	if ((error = avformat_find_stream_info(m_input_fmt_ctx, NULL)) < 0) {
		fprintf(stderr, "Could not open find stream info (error '%s')\n",
			get_error_text(error));
		avformat_close_input(&m_input_fmt_ctx);
		return error;
	}

	/** Make sure that there is only one stream in the input file. */
	if (m_input_fmt_ctx->nb_streams != 1) {
		fprintf(stderr, "Expected one audio input stream, but found %d\n",
			m_input_fmt_ctx->nb_streams);
		avformat_close_input(&m_input_fmt_ctx);
		return AVERROR_EXIT;
	}

	/** Find a decoder for the audio stream. */
	if (!(input_codec = avcodec_find_decoder(m_input_fmt_ctx->streams[0]->codec->codec_id))) {
		fprintf(stderr, "Could not find input codec\n");
		avformat_close_input(&m_input_fmt_ctx);
		return AVERROR_EXIT;
	}
	/** Open the decoder for the audio stream to use it later. */
	if ((error = avcodec_open2(m_input_fmt_ctx->streams[0]->codec,
		input_codec, NULL)) < 0) {
			fprintf(stderr, "Could not open input codec (error '%s')\n",
				get_error_text(error));
			avformat_close_input(&m_input_fmt_ctx);
			return error;
	}

	/** Save the decoder context for easier access later. */
	m_input_codec_ctx = m_input_fmt_ctx->streams[0]->codec;
	return 0;
}

/**
 * Initialize the audio resampler based on the input and output codec settings.
 * If the input and output sample formats differ, a conversion is required
 * libswresample takes care of this, but requires initialization.
 */
int CAVDecode::init_resampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context)
{
    int error;
    /**
        * Create a resampler context for the conversion.
        * Set the conversion parameters.
        * Default channel layouts based on the number of channels
        * are assumed for simplicity (they are sometimes not detected
        * properly by the demuxer and/or decoder).
        */
    m_Resample_ctx = swr_alloc_set_opts(NULL,
			av_get_default_channel_layout(output_codec_context->channels),
			output_codec_context->sample_fmt,
			output_codec_context->sample_rate,
			av_get_default_channel_layout(input_codec_context->channels),
			input_codec_context->sample_fmt,
			input_codec_context->sample_rate,
			0, NULL);
    if (!m_Resample_ctx) {
        fprintf(stderr, "Could not allocate resample context\n");
        return AVERROR(ENOMEM);
    }
    /**
    * Perform a sanity check so that the number of converted samples is
    * not greater than the number of samples to be converted.
    * If the sample rates differ, this case has to be handled differently
    */
    //av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);
    /** Open the resampler with the specified parameters. */
    if ((error = swr_init(m_Resample_ctx)) < 0) {
        fprintf(stderr, "Could not open resample context\n");
        swr_free(&m_Resample_ctx);
        return error;
    }
	m_output_codec_ctx = output_codec_context;
    return 0;
}

int CAVDecode::init_converted_samples( uint8_t ***converted_input_samples, AVCodecContext *output_codec_context, 
	int frame_size, int *dst_nb_samples, int *dst_linesize )
{
	int error;
    /**
     * Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*converted_input_samples = (uint8_t **)calloc(output_codec_context->channels,sizeof(**converted_input_samples)))) 
	{
        fprintf(stderr, "Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

	(*dst_nb_samples) = static_cast<int>(av_rescale_rnd(swr_get_delay(m_Resample_ctx, m_input_codec_ctx->sample_rate) +
		frame_size, output_codec_context->sample_rate, m_input_codec_ctx->sample_rate, AV_ROUND_ZERO));

	/**
     * Allocate memory for the samples of all channels in one consecutive
     * block for convenience.
     */
    if ((error = av_samples_alloc(*converted_input_samples, dst_linesize,
	                              output_codec_context->channels,
                                  *dst_nb_samples,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        fprintf(stderr,
                "Could not allocate converted input samples (error '%s')\n",
                get_error_text(error));
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}

int CAVDecode::convert_samples( const uint8_t **input_data, const int input_nb_samples,
	uint8_t **converted_data, const int output_nb_samples, SwrContext *resample_context )
{
	int nb_samples;
	/** Convert the samples using the resampler. */
	if ((nb_samples = swr_convert(resample_context, converted_data, 
		output_nb_samples, input_data, input_nb_samples)) < 0) 
	{
		fprintf(stderr, "Could not convert input samples (error '%s')\n",
			get_error_text(nb_samples));
	}
	return nb_samples;
}

int CAVDecode::add_samples_to_fifo( AVAudioFifo *fifo, uint8_t **converted_input_samples, const int frame_size )
{
	int error;
    /**
     * Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples.
     */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }
    /** Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
                            frame_size) < frame_size) {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}

int CAVDecode::decode_convert_and_store( AVAudioFifo *fifo, int *finished )
{
	/** Temporary storage of the input samples of the frame read from the file. */
    AVFrame *input_frame = NULL;
    /** Temporary storage for the converted input samples. */
    uint8_t **converted_input_samples = NULL;
    int data_present;
    int ret = AVERROR_EXIT;
    /** Initialize temporary storage for one input frame. */
    if (init_input_frame(&input_frame))
        goto cleanup;
    /** Decode one frame worth of audio samples. */
    if (decode_audio_frame(input_frame, m_input_fmt_ctx,
                           m_input_codec_ctx, &data_present, finished))
        goto cleanup;
    /**
     * If we are at the end of the file and there are no more samples
     * in the decoder which are delayed, we are actually finished.
     * This must not be treated as an error.
     */
    if (*finished && !data_present) {
        ret = 0;
        goto cleanup;
    }
    /** If there is decoded data, convert and store it */
    if (data_present) {
        /** Initialize the temporary storage for the converted input samples. */
		int dst_nb_samples = 0, dst_linesize = 0;
        if (init_converted_samples(&converted_input_samples, m_output_codec_ctx,
                                   input_frame->nb_samples, &dst_nb_samples, &dst_linesize))
            goto cleanup;
        /**
         * Convert the input samples to the desired output sample format.
         * This requires a temporary storage provided by converted_input_samples.
         */
        if ((ret = convert_samples((const uint8_t**)input_frame->extended_data, input_frame->nb_samples, converted_input_samples,
                            dst_nb_samples, m_Resample_ctx)) < 0)
            goto cleanup;
        
        if (add_samples_to_fifo(fifo, converted_input_samples, ret))
            goto cleanup;
        ret = 0;
    }
    ret = 0;

cleanup:
    if (converted_input_samples) {
        av_freep(&converted_input_samples[0]);
        free(converted_input_samples);
    }
    av_frame_free(&input_frame);
    return ret;
}

int CAVDecode::decode_audio_frame( AVFrame *frame, AVFormatContext *input_format_context, AVCodecContext *input_codec_context, int *data_present, int *finished )
{
	    /** Packet used for temporary storage. */
    AVPacket input_packet;
    int error;
    init_packet(&input_packet);
    /** Read one audio frame from the input file into a temporary packet. */
    if ((error = av_read_frame(input_format_context, &input_packet)) < 0) {
        /** If we are the the end of the file, flush the decoder below. */
        if (error == AVERROR_EOF)
            *finished = 1;
        else {
            fprintf(stderr, "Could not read frame (error '%s')\n",
                    get_error_text(error));
            return error;
        }
    }
    /**
     * Decode the audio frame stored in the temporary packet.
     * The input audio stream decoder is used to do this.
     * If we are at the end of the file, pass an empty packet to the decoder
     * to flush it.
     */
    if ((error = avcodec_decode_audio4(input_codec_context, frame,
                                       data_present, &input_packet)) < 0) {
        fprintf(stderr, "Could not decode frame (error '%s')\n",
                get_error_text(error));
        av_free_packet(&input_packet);
        return error;
    }
    /**
     * If the decoder has not been flushed completely, we are not finished,
     * so that this function has to be called again.
     */
    if (*finished && *data_present)
        *finished = 0;

    av_free_packet(&input_packet);
    return 0;
}

int CAVDecode::init_input_frame( AVFrame **frame )
{
	if (!(*frame = av_frame_alloc())) {
		fprintf(stderr, "Could not allocate input frame\n");
		return AVERROR(ENOMEM);
	}
	return 0;
}

void CAVDecode::init_packet( AVPacket *packet )
{
	av_init_packet(packet);
	/** Set the packet data and size so that it is recognized as being empty. */
	packet->data = NULL;
	packet->size = 0;
}