#pragma once
#ifndef AVDECODE_H_
#define AVDECODE_H_

#include "FFmpeg.h"

class CAVDecode
{
public:
	CAVDecode(void);
	virtual ~CAVDecode(void);
	CAVDecode(const char* file, AVCodecContext *output_codec_context, AVInputFormat *iformat = NULL);
	
	/**
	 * Read one audio frame from the input file, decodes, converts and stores
	 * it in the FIFO buffer.
	 */
	int decode_convert_and_store( AVAudioFifo *fifo, int *finished );

	AVCodecContext *getCodecContext(){ return m_input_codec_ctx;}
	SwrContext *getSwrContext(){ return m_Resample_ctx;}

private:
	/** Decode one audio frame from the input file. */
	int decode_audio_frame(AVFrame *frame,
		AVFormatContext *input_format_context,
		AVCodecContext *input_codec_context,
		int *data_present, int *finished);

	/** Open an input file and the required decoder. */
	int initialize(const char *file, AVInputFormat *iformat = NULL);

	/**
	* Initialize the audio resampler based on the input and output codec settings.
	* If the input and output sample formats differ, a conversion is required
	* libswresample takes care of this, but requires initialization.
	*/
	int init_resampler(AVCodecContext *input_codec_context,
				AVCodecContext *output_codec_context);

	/**
	* Initialize a temporary storage for the specified number of audio samples.
	* The conversion requires temporary storage due to the different format.
	* The number of audio samples to be allocated is specified in frame_size.
	*/
	int init_converted_samples( uint8_t ***converted_input_samples, AVCodecContext *output_codec_context, 
				int frame_size, int *dst_nb_samples, int *dst_linesize );
	/**
	* Convert the input audio samples into the output sample format.
	* The conversion happens on a per-frame basis, the size of which is specified
	* by frame_size.
	*/
	int convert_samples( const uint8_t **input_data, const int frame_size, uint8_t **converted_data, 
				const int output_size, SwrContext *resample_context );

	/** Add converted input audio samples to the FIFO buffer for later processing. */
	int add_samples_to_fifo(AVAudioFifo *fifo,
				uint8_t **converted_input_samples,
				const int frame_size);

	/** Initialize one audio frame for reading from the input file */
	int init_input_frame(AVFrame **frame);

	/** Initialize one data packet for reading or writing. */
	void init_packet(AVPacket *packet);

	inline char *const get_error_text(const int error)
	{
		static char error_buffer[255];
		av_strerror(error, error_buffer, sizeof(error_buffer));
		return error_buffer;
	}

private:
	AVFormatContext *m_input_fmt_ctx;
	AVCodecContext *m_input_codec_ctx;
	SwrContext *m_Resample_ctx;
	AVCodecContext *m_output_codec_ctx;
};

#endif /* AVDECODE_HPP_ */