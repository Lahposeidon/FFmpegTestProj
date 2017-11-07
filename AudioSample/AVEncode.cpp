#include "StdAfx.h"
#include "AVEncode.h"
#include <stdexcept>

#include "FFmpeg.h"

#define DEF_CHANNELS                    2
#define DEF_SAMPLES_RATE                44100
#define DEF_BIT_RATE					64000
#define DEF_SAMPLE_FMT                  AV_SAMPLE_FMT_S16P

CAVEncode::CAVEncode(void)
{
}

CAVEncode::CAVEncode( char* szPath, AVAudioFifo **fifo )
{
	if(open_output_file(szPath) >= 0)
	{
		init_fifo(fifo);
	}	
}

CAVEncode::~CAVEncode(void)
{
	if (m_output_codec_ctx)
		avcodec_close(m_output_codec_ctx);

	if (m_output_fmt_ctx) 
	{
		avio_close(m_output_fmt_ctx->pb);
		avformat_free_context(m_output_fmt_ctx);
	}
}

int CAVEncode::start()
{
	int r = 0;
	if ((strrchr(m_output_fmt_ctx->filename, '\\')) == NULL)
		r = av_dict_set( &m_output_fmt_ctx->metadata, "title", m_output_fmt_ctx->filename, AV_DICT_APPEND);
	else
		r = av_dict_set( &m_output_fmt_ctx->metadata, "title", strrchr( m_output_fmt_ctx->filename, '\\') + 1, AV_DICT_APPEND);

	r = av_dict_set( &m_output_fmt_ctx->metadata, "album", "PODCAST", AV_DICT_APPEND);
	//r = av_dict_set( &m_output_fmt_ctx->metadata, "Comments", "Build for W corporation", AV_DICT_APPEND);
	r = av_dict_set( &m_output_fmt_ctx->metadata, "genre", "podcast", AV_DICT_APPEND);
	r = av_dict_set( &m_output_fmt_ctx->metadata, "publisher", "JTBC", AV_DICT_APPEND);
	r = av_dict_set( &m_output_fmt_ctx->metadata, "encoded_by", "W corporation", AV_DICT_APPEND);
	
	r = avformat_write_header( m_output_fmt_ctx, &m_output_fmt_ctx->metadata);

	return r;
}

void CAVEncode::end()
{
	write_output_file_trailer();
}

int CAVEncode::open_output_file( const char *filename )
{
	AVIOContext *output_io_context = NULL;
    AVStream *stream               = NULL;
    AVCodec *output_codec          = NULL;
    int error;
    /** Open the output file to write to it. */
    if ((error = avio_open(&output_io_context, filename,
                           AVIO_FLAG_WRITE)) < 0) {
        fprintf(stderr, "Could not open output file '%s' (error '%s')\n",
                filename, get_error_text(error));
        return error;
    }
    /** Create a new format context for the output container format. */
    if (!(m_output_fmt_ctx = avformat_alloc_context())) {
        fprintf(stderr, "Could not allocate output format context\n");
        return AVERROR(ENOMEM);
    }
    /** Associate the output file (pointer) with the container format context. */
    m_output_fmt_ctx->pb = output_io_context;
    /** Guess the desired container format based on the file extension. */
    if (!(m_output_fmt_ctx->oformat = av_guess_format(NULL, filename,NULL)))
	{
        fprintf(stderr, "Could not find output file format\n");
        goto cleanup;
    }

    av_strlcpy(m_output_fmt_ctx->filename, filename, sizeof(m_output_fmt_ctx->filename));

    /** Find the encoder to be used by its name. */
    if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_MP3))) {
        fprintf(stderr, "Could not find an AAC encoder.\n");
        goto cleanup;
    }
    /** Create a new audio stream in the output file container. */
    if (!(stream = avformat_new_stream(m_output_fmt_ctx, output_codec))) {
        fprintf(stderr, "Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }
    /** Save the encoder context for easier access later. */
    m_output_codec_ctx = stream->codec;
    /**
     * Set the basic encoder parameters.
     * The input file's sample rate is used to avoid a sample rate conversion.
     */
    m_output_codec_ctx->channels       = DEF_CHANNELS;
    m_output_codec_ctx->channel_layout = av_get_default_channel_layout(DEF_CHANNELS);
    m_output_codec_ctx->sample_rate    = DEF_SAMPLES_RATE;
    m_output_codec_ctx->sample_fmt     = DEF_SAMPLE_FMT;
    m_output_codec_ctx->bit_rate       = DEF_BIT_RATE;
    /**
     * Some container formats (like MP4) require global headers to be present
     * Mark the encoder so that it behaves accordingly.
     */
    if (m_output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        m_output_fmt_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    /** Open the encoder for the audio stream to use it later. */
    if ((error = avcodec_open2(m_output_codec_ctx, output_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec (error '%s')\n",
                get_error_text(error));
        goto cleanup;
    }

	if(start() < 0)
		goto cleanup;

    return 0;
cleanup:
    avio_close(m_output_fmt_ctx->pb);
    avformat_free_context(m_output_fmt_ctx);
    m_output_fmt_ctx = NULL;
    return error < 0 ? error : AVERROR_EXIT;
}

int CAVEncode::init_output_frame( AVFrame **frame, AVCodecContext *output_codec_context, int frame_size )
{
	int error;
    /** Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate output frame\n");
        return AVERROR_EXIT;
    }
    /**
     * Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity.
     */
    (*frame)->nb_samples     = frame_size;
    (*frame)->channel_layout = output_codec_context->channel_layout;
    (*frame)->format         = output_codec_context->sample_fmt;
    (*frame)->sample_rate    = output_codec_context->sample_rate;
    /**
     * Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified.
     */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        fprintf(stderr, "Could allocate output frame samples (error '%s')\n",
                get_error_text(error));
        av_frame_free(frame);
        return error;
    }
    return 0;
}

int CAVEncode::load_encode_and_write( AVAudioFifo *fifo )
{
	/** Temporary storage of the output samples of the frame written to the file. */
    AVFrame *output_frame;
    /**
     * Use the maximum number of possible samples per frame.
     * If there is less than the maximum possible frame size in the FIFO
     * buffer use this number. Otherwise, use the maximum possible frame size
     */
    const int frame_size = FFMIN(av_audio_fifo_size(fifo),
                                 m_output_codec_ctx->frame_size);
    int data_written;
    /** Initialize temporary storage for one output frame. */
    if (init_output_frame(&output_frame, m_output_codec_ctx, frame_size))
        return AVERROR_EXIT;
    /**
     * Read as many samples from the FIFO buffer as required to fill the frame.
     * The samples are stored in the frame temporarily.
     */
    if (av_audio_fifo_read(fifo, (void **)output_frame->data, frame_size) < frame_size) {
        fprintf(stderr, "Could not read data from FIFO\n");
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }
    /** Encode one frame worth of audio samples. */
    if (encode_audio_frame(output_frame, &data_written)) {
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }
    av_frame_free(&output_frame);
    return 0;
}

int CAVEncode::encode_audio_frame( AVFrame *frame, int *data_present )
{
	/** Packet used for temporary storage. */
    AVPacket output_packet;
    int error;
    init_packet(&output_packet);
    /**
     * Encode the audio frame and store it in the temporary packet.
     * The output audio stream encoder is used to do this.
     */
    if ((error = avcodec_encode_audio2(m_output_codec_ctx, &output_packet,
                                       frame, data_present)) < 0) {
        fprintf(stderr, "Could not encode frame (error '%s')\n",
                get_error_text(error));
        av_free_packet(&output_packet);
        return error;
    }
    /** Write one audio frame from the temporary packet to the output file. */
    if (*data_present) {
        if ((error = av_write_frame(m_output_fmt_ctx, &output_packet)) < 0) {
            fprintf(stderr, "Could not write frame (error '%s')\n",
                    get_error_text(error));
            av_free_packet(&output_packet);
            return error;
        }
        av_free_packet(&output_packet);
    }
    return 0;
}

int CAVEncode::write_output_file_trailer()
{
	int error;
	if ((error = av_write_trailer(m_output_fmt_ctx)) < 0) {
		fprintf(stderr, "Could not write output file trailer (error '%s')\n",
			get_error_text(error));
		return error;
	}
	return 0;
}

void CAVEncode::init_packet( AVPacket *packet )
{
	av_init_packet(packet);
	/** Set the packet data and size so that it is recognized as being empty. */
	packet->data = NULL;
	packet->size = 0;
}

int CAVEncode::init_fifo( AVAudioFifo **fifo )
{
	/** Create the FIFO buffer based on the specified output sample format. */
	if (!(*fifo = av_audio_fifo_alloc(m_output_codec_ctx->sample_fmt, m_output_codec_ctx->channels, 1))) {
		fprintf(stderr, "Could not allocate FIFO\n");
		return AVERROR(ENOMEM);
	}
	return 0;
}
