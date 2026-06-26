// Copyright © 2015 CCP ehf.

#pragma once
#ifndef IVideoDecoder_H
#define IVideoDecoder_H

#include "Metadata.h"

// --------------------------------------------------------------------------------------
// Description:
//   A base video decoder interface. The SetDropFrameTime is called with the time limit
//   for frames: undecoded frames with a timestamp earlier than this time can be dropped.
// See also:
//   VpxDecoder
// --------------------------------------------------------------------------------------
struct IVideoDecoder
{
	virtual ~IVideoDecoder() = 0;
	virtual VideoFrameQueue& GetDecodedQueue() = 0;
	virtual void SetDropFrameTime( uint64_t time ) = 0;
	virtual DecoderError GetError() const = 0;
};

// --------------------------------------------------------------------------------------
// Description:
//   Constructs a video decoder from the given video metadata.
// --------------------------------------------------------------------------------------
IVideoDecoder* CreateVideoDecoder( const VideoMetadata& videoMetadata, EncodedVideoFrameQueue& encodedQueue );

#endif