// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "IAudioDecoder.h"
#include "VorbisDecoder.h"

IAudioDecoder::~IAudioDecoder()
{
}

IAudioDecoder* CreateAudioDecoder( const AudioMetadata& audioMetadata, EncodedAudioFrameQueue& compressedQueue )
{
	switch( audioMetadata.codec )
	{
	case AudioMetadata::VORBIS:
		return CCP_NEW( "CreateAudioDecoder/VorbisDecoder" ) VorbisDecoder( audioMetadata, compressedQueue );
	default:
		return nullptr;
	}
}