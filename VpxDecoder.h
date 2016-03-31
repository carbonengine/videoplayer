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
	void ConvertThread();


	struct YuvFrame
	{
		std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> y;
		std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> u;
		std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> v;
		std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> alpha;

		int yStride;
		int uStride;
		int vStride;
		int alphaStride;
		uint32_t width;
		uint32_t height; 
		uint64_t timeStamp;
		uint32_t uvShift;
	};

	FrameQueue<YuvFrame, MaxCountFullPolicy> m_yuvFrameQueue;


	EncodedVideoFrameQueue& m_compressedQueue;
	VideoFrameQueue m_decompressedQueue;
	vpx_codec_ctx_t m_videoCodec;
	vpx_codec_ctx_t m_alphaCodec;
	const VideoMetadata& m_videoMetadata;
	uint64_t m_dropFrameTime;
	bool m_stopRequested;
	CcpThread m_decodeThread;
	CcpThread m_convertThread;
	DecoderError m_error;
	uint32_t m_processedFrames;
	uint32_t m_corruptFrames;
	uint32_t m_droppedFrames;
};

#endif