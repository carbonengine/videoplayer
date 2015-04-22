////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef Audio2Sink_H
#define Audio2Sink_H

#include "IAudioSinkExposed.h"
#include "Timer.h"


// --------------------------------------------------------------------------------------
// Description:
//   An audio sink/player that passes data to audio2 library for playing. 
// See also:
//   IAudioSinkExposed
// --------------------------------------------------------------------------------------
BLUE_CLASS( Audio2Sink ): 
	public IAudioSinkExposed,
	public IBlueEvents
{
public:
	Audio2Sink();
	~Audio2Sink();

	EXPOSE_TO_BLUE();

	void Create( intptr_t audioFunction, int32_t outputChannel );

	virtual void Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue );
	virtual void Close();
	virtual void Pause();
	virtual void Resume();
	virtual uint64_t GetTime();
	virtual bool IsDone();

	virtual void OnTick( Be::Time realTime, Be::Time simTime, void* cookie );
private:
	typedef unsigned int (__stdcall * SetAudioStream)( float* const data, unsigned int const dataSize, int const input );

	void SubmitThread();

	SetAudioStream m_setAudioStream;
	int32_t m_outputChannel;

	const AudioMetadata* m_audioMetadata;
	PcmFrameQueue* m_frameQueue;

	CcpThread m_submitThread;
	Timer m_timer;
	uint64_t m_length;
	bool m_stopRequested;
	bool m_finishedSubmitting;

	static const uint32_t BUFFER_SIZE = 16384;
	float m_buffers[2][16384];
	uint32_t m_bufferSizes[2];
	CcpAtomic<uint32_t> m_bufferReady;
};

TYPEDEF_BLUECLASS( Audio2Sink );


#endif