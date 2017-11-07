#pragma once
#ifndef AVENCODE_H_
#define AVENCODE_H_

#include "FFmpeg.h"

class CAVEncode
{
public:
	CAVEncode(void);
	virtual ~CAVEncode(void);
	CAVEncode(char* szPath, AVAudioFifo **fifo);

	/**
	* Load one audio frame from the FIFO buffer, encode and write it to the
	* output file.
	*/
	int load_encode_and_write( AVAudioFifo *fifo );

	/** Encode one frame worth of audio to the output file. */
	int encode_audio_frame( AVFrame *frame, int *data_present );

	int start();
	void end();

	AVCodecContext *getCodecContext(){ return m_output_codec_ctx;}

private:
	/** Initialize a FIFO buffer for the audio samples to be encoded. */
	int init_fifo(AVAudioFifo **fifo);

	/** Write the trailer of the output file container. */
	int write_output_file_trailer();

	/**
	* Open an output file and the required encoder.
	* Also set some basic encoder parameters.
	* Some of these parameters are based on the input file's parameters.
	*/
	int open_output_file( const char *filename );

	/**
	* Initialize one input frame for writing to the output file.
	* The frame will be exactly frame_size samples large.
	*/
	int init_output_frame(AVFrame **frame,
			AVCodecContext *output_codec_context,
			int frame_size);

	/** Initialize one data packet for reading or writing. */
	void init_packet(AVPacket *packet);

	inline char *const get_error_text(const int error)
	{
		static char error_buffer[255];
		av_strerror(error, error_buffer, sizeof(error_buffer));
		return error_buffer;
	}

private:
	AVFormatContext *m_output_fmt_ctx;
	AVCodecContext *m_output_codec_ctx;
};

#endif /* AVENCODE_HPP_ */