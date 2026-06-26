// Copyright © 2015 CCP ehf.

#pragma once
#ifndef IAudioDecoder_H
#define IAudioDecoder_H

#include "Metadata.h"

// --------------------------------------------------------------------------------------
// Description:
//   A base audio decoder interface.
// --------------------------------------------------------------------------------------
struct IAudioDecoder
{
	virtual ~IAudioDecoder() = 0;
	virtual PcmFrameQueue& GetDecodedQueue() = 0;
	virtual DecoderError GetError() const = 0;
};

// --------------------------------------------------------------------------------------
// Description:
//   Constructs an audio decoder from the given audio metadata.
// --------------------------------------------------------------------------------------
IAudioDecoder* CreateAudioDecoder( const AudioMetadata& audioMetadata, EncodedAudioFrameQueue& compressedQueue );

#endif