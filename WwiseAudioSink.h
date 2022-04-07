////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Eric Nielsen
// Created:		August 2020
// Copyright:	CCP 2020
// Description: An audio sink for the videoplayer that pipes its audio buffer into Wwises
//				Audio Input plugin (See AudioInputMgr in audio2 for more details on how the Audio
//				Input plugin works).

#pragma once
#ifndef WwiseAudioSink_H
#define WwiseAudioSink_H

#include "stdafx.h"
#include "IAudioSinkExposed.h"
#include "audio2/IAudioInputMgr.h"
#include "Timer.h"

BLUE_CLASS( WwiseAudioSink ) :
	public IAudioSinkExposed,
	public IAudioInputSink
{
public:
	WwiseAudioSink();
	~WwiseAudioSink();

	EXPOSE_TO_BLUE();

	// IAudioSinkExposed
	virtual void Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue ) override;
	virtual void Close() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual uint64_t GetTime() override;
	virtual bool IsDone() override;

	// IAudioInputSink
	int FillBuffer( IAudioInputMgr::BufferData& ) override;

	// WwiseAudioSink
	void Create( IAudioInputMgr * inputMgr );
	void SetVolume( float volume );
	float GetVolume();

private:
	void FinishPlaying();

	static const uint64_t NSEC_TO_SEC = 1000000000; // Nano seconds to seconds.

	const AudioMetadata* m_audioMetadata;
	PcmFrameQueue* m_frameQueue;
	bool m_done;
	bool m_playing;
	bool m_stopRequested;
	uint64_t m_samplesPlayed;
	uint64_t m_samplesSubmitted;
	int m_pauseCounter;
	float m_volume;
	Timer m_timer;

	IAudioInputMgrPtr m_audioInputMgr;
};

TYPEDEF_BLUECLASS( WwiseAudioSink );

#endif
