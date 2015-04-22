////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef VorbisDecoder_H
#define VorbisDecoder_H

#include "FrameQueue.h"
#include "Metadata.h"
#include "IAudioDecoder.h"
#include "vorbis/codec.h"

// --------------------------------------------------------------------------------------
// Description:
//   A decoder for ogg/vorbis audio.
// See also:
//   IAudioDecoder
// --------------------------------------------------------------------------------------
class VorbisDecoder: public IAudioDecoder
{
public:
	VorbisDecoder( const AudioMetadata& audioMetadata, EncodedAudioFrameQueue& compressedQueue );
	~VorbisDecoder();

	PcmFrameQueue& GetDecodedQueue();

	DecoderError GetError() const;
	uint32_t GetProcessedFrameCount() const;
	uint32_t GetCorruptFrameCount() const;

	void WaitUntilDone();
private:
	enum InitializeState
	{
		NONE,
		INFO,
		BLOCK,
	};
	void DecodeThread();

	EncodedAudioFrameQueue& m_compressedQueue;
	PcmFrameQueue m_decodedQueue;
	const AudioMetadata& m_audioMetadata;

	vorbis_info m_vorbisInfo;
	vorbis_comment m_vorbisComment;
	vorbis_dsp_state m_vorbisDsp;
	vorbis_block m_vorbisBlock;
	InitializeState m_initializeState;
	bool m_stopRequested;
	CcpThread m_decodeThread;
	DecoderError m_error;
	uint32_t m_processedFrames;
	uint32_t m_corruptFrames;
};

#endif