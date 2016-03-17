////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef Metadata_H
#define Metadata_H

#include "FrameQueue.h"

// --------------------------------------------------------------------------------------
// Description:
//   Video stream metadata.
// --------------------------------------------------------------------------------------
struct VideoMetadata
{
	enum Codec
	{
		OTHER,
		VP8,
		VP9,
	};
	Codec codec;
	uint32_t width;
	uint32_t height;
	bool hasAlpha;

	VideoMetadata( Codec codec_ = OTHER, uint32_t width_ = 0, uint32_t height_ = 0, bool hasAlpha_ = false )
		:codec( codec_ ),
		width( width_ ),
		height( height_ ),
		hasAlpha( hasAlpha_ )
	{
	}
};

// --------------------------------------------------------------------------------------
// Description:
//   Audio stream metadata.
// --------------------------------------------------------------------------------------
struct AudioMetadata
{
	enum Codec
	{
		OTHER,
		VORBIS,
	};
	Codec codec;
	uint32_t channels;
	uint32_t bps;
	uint32_t rate;

	struct CodecInitializationData
	{
		std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> data;
		size_t length;

		CodecInitializationData()
			:length( 0 )
		{
		}

		CodecInitializationData( const CodecInitializationData& other )
			:length( other.length )
		{
			if( length )
			{
				data.reset( CCP_NEW( "CodecInitializationData/data" ) uint8_t[length] );
				memcpy( data.get(), other.data.get(), length );
			}
		}

		CodecInitializationData( const uint8_t* data_, size_t length_ )
			:length( length_ )
		{
			if( length )
			{
				data.reset( CCP_NEW( "CodecInitializationData/data" ) uint8_t[length] );
				memcpy( data.get(), data_, length );
			}
		}

		CodecInitializationData& operator=( const CodecInitializationData& other )
		{
			if( this == &other )
			{
				return *this;
			}
			length = other.length;
			if( other.data )
			{
				data.reset( CCP_NEW( "CodecInitializationData/data" ) uint8_t[length] );
				memcpy( data.get(), other.data.get(), length );
			}
			else
			{
				data.reset();
			}
			return *this;
		}
	};
	CodecInitializationData codecInitializationData[3];

	AudioMetadata( 
		Codec codec_ = OTHER, 
		uint32_t channels_ = 0, 
		uint32_t bps_ = 0, 
		uint32_t rate_ = 0, 
		CodecInitializationData data0 = CodecInitializationData(), 
		CodecInitializationData data1 = CodecInitializationData(), 
		CodecInitializationData data2 = CodecInitializationData() )
		:codec( codec_ ),
		channels( channels_ ),
		bps( bps_ ),
		rate( rate_ )
	{
		codecInitializationData[0] = data0;
		codecInitializationData[1] = data1;
		codecInitializationData[2] = data2;
	}
};

// --------------------------------------------------------------------------------------
// Description:
//   A frame of uncompressed PCM audio. Audio decoders emit these frames for audio sinks
//   to consume.
// --------------------------------------------------------------------------------------
struct PcmFrame
{
	uint64_t timeStamp;
	uint32_t samples;
	uint32_t channels;
	std::unique_ptr<int16_t[], TrackableDelete<int16_t[]>> data;
};

typedef FrameQueue<PcmFrame, MaxCountFullPolicy> PcmFrameQueue;

// --------------------------------------------------------------------------------------
// Description:
//   A frame of uncompressed video in YCuCv format. Video decoders emit these frames.
// --------------------------------------------------------------------------------------
struct VideoFrame
{
	uint32_t yWidth;
	uint32_t yHeight;
	uint32_t uvWidth;
	uint32_t uvHeight;
	uint64_t timeStamp;
	std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> y;
	std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> u;
	std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> v;
	std::unique_ptr<uint8_t[], TrackableDelete<uint8_t[]>> alpha;
};

typedef FrameQueue<VideoFrame, MaxCountFullPolicy> VideoFrameQueue;

// --------------------------------------------------------------------------------------
// Description:
//   A base interface for encoded frames (both audio and video).
// --------------------------------------------------------------------------------------
struct IEncodedFrame
{
	virtual ~IEncodedFrame() = 0;
	virtual size_t GetFrameCount() = 0;
	virtual uint64_t GetTimeStamp() = 0;
	virtual bool GetFrame( size_t index, uint8_t*& data, size_t& length ) = 0;
	virtual bool GetAlphaFrame( uint8_t*& data, size_t& length ) = 0;
};

typedef FrameQueue<IEncodedFrame, MaxIntervalPolicy> EncodedAudioFrameQueue;
typedef FrameQueue<IEncodedFrame, MaxCountFullPolicy> EncodedVideoFrameQueue;


enum ParserError
{
	PARSER_ERROR_OK,
	PARSER_ERROR_INVALID_STREAM,
	PARSER_ERROR_INVALID_DATA,
};

enum DecoderError
{
	DECODER_ERROR_OK,
	DECODER_ERROR_UNSUPPORTED_CODEC,
};

enum StreamType
{
	STREAM_AUDIO		= 1,
	STREAM_VIDEO		= 2,
	STREAM_AUDIO_VIDEO	= 3,
};

#endif