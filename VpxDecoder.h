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
//   Decoded video frame in YUV format.
// --------------------------------------------------------------------------------------
struct YuvFrame
{
	YuvFrame( uint32_t width, uint32_t height, uint32_t uvShift, bool alpha );

	static void* operator new( std::size_t sz, uint32_t width, uint32_t height, uint32_t uvShift, bool alpha );
	static void operator delete( void* ptr );

	uint8_t* y;
	uint8_t* u;
	uint8_t* v;
	uint8_t* alpha;

	uint32_t width;
	uint32_t height; 
	uint64_t timeStamp;
	uint32_t uvShift;
};

// --------------------------------------------------------------------------------------
// Description:
//   A decoder for VP8/VP9 video.
// See also:
//   IVideoDecoder
// --------------------------------------------------------------------------------------
class VpxDecoder: public IVideoDecoder, public FrameOwner<YuvFrame>, public FrameOwner<VideoFrame>
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

	YuvFrame* GetNewYuvFrame( uint32_t width, uint32_t height, uint32_t uvShift, bool alpha );
	VideoFrame* GetNewVideoFrame( uint32_t width, uint32_t height );
	void ReleaseFrame( YuvFrame* frame );
	void ReleaseFrame( VideoFrame* frame );

	std::vector<std::unique_ptr<YuvFrame>> m_yuvFramePool;
	std::vector<std::unique_ptr<VideoFrame>> m_videoFramePool;
	CcpMutex m_poolMutex;

	FrameQueue<YuvFrame, MaxCountFullPolicy, FrameDeleter<YuvFrame>> m_yuvFrameQueue;
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