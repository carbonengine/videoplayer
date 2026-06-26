// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "IVideoDecoder.h"
#include "VpxDecoder.h"

IVideoDecoder::~IVideoDecoder()
{
}

IVideoDecoder* CreateVideoDecoder( const VideoMetadata& videoMetadata, EncodedVideoFrameQueue& encodedQueue )
{
	switch( videoMetadata.codec )
	{
	case VideoMetadata::VP8:
	case VideoMetadata::VP9:
		return CCP_NEW( "CreateVideoDecoder/VpxDecoder" ) VpxDecoder( videoMetadata, encodedQueue );
	default:
		return nullptr;
	}
}