////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef IAudioSink_H
#define IAudioSink_H

#include "Metadata.h"

// --------------------------------------------------------------------------------------
// Description:
//   A base audio sink (player) interface. Audio sink is supposed to continiously pull
//   audio frames from the queue and present them to the audio device or API. It also
//   acts as a timer: if audio sink is present VideoController uses its GetTime function
//   to syncronize video to audio.
//   Pause and Resume methods are used for pausing the playback. They must be implemented
//   in a reference-counting manner.
// See also:
//   WaveOutAudioSink
// --------------------------------------------------------------------------------------
struct IAudioSink
{
	virtual ~IAudioSink() = 0;
	virtual void Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue ) = 0;
	virtual void Close() = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
	virtual uint64_t GetTime() = 0;
	virtual bool IsDone() = 0;
};

#endif