////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef VpxDecoder_H
#define VpxDecoder_H

#include "FrameQueue.h"
#include "Metadata.h"
#include "vpx_decoder.h"
#include "IVideoDecoder.h"

// --------------------------------------------------------------------------------------
// Description:
//   A decoder for VP8/VP9 video.
// See also:
//   IVideoDecoder
// --------------------------------------------------------------------------------------
class VpxDecoder: public IVideoDecoder
{
public:
	VpxDecoder( const VideoMetadata& videoMetadata, EncodedVideoFrameQueue& compressedQueue );
	~VpxDecoder();

	VideoFrameQueue& GetDecodedQueue();

	void SetDropFrameTime( uint64_t time );
	DecoderError GetError() const;
	uint32_t GetProcessedFrameCount() const;
	uint32_t GetCorruptFrameCount() const;
	uint32_t GetDroppedFrameCount() const;
	void WaitUntilDone();
private:
	void DecodeThread();

	EncodedVideoFrameQueue& m_compressedQueue;
	VideoFrameQueue m_decompressedQueue;
	vpx_codec_ctx_t m_videoCodec;
	const VideoMetadata& m_videoMetadata;
	uint64_t m_dropFrameTime;
	bool m_stopRequested;
	CcpThread m_decodeThread;
	DecoderError m_error;
	uint32_t m_processedFrames;
	uint32_t m_corruptFrames;
	uint32_t m_droppedFrames;
};

#endif