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

	void Create( int64_t audioFunction, int64_t audioPositionFunction, int32_t outputChannel );

	virtual void Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue );
	virtual void Close();
	virtual void Pause();
	virtual void Resume();
	virtual uint64_t GetTime();
	virtual bool IsDone();

	virtual void OnTick( Be::Time realTime, Be::Time simTime, void* cookie );
private:
#ifdef _WIN32
	typedef unsigned int (__stdcall * SetAudioStream)( float* const data, unsigned int const dataSize, int const input );
	typedef unsigned int (__stdcall * GetAudioStreamPosition)( int const input );
#else
    typedef unsigned int (*SetAudioStream)( float* const data, unsigned int const dataSize, int const input );
	typedef unsigned int (*GetAudioStreamPosition)( int const input );
#endif
	void SubmitThread();

	SetAudioStream m_setAudioStream;
	GetAudioStreamPosition m_getAudioStreamPosition;
	int32_t m_outputChannel;

	const AudioMetadata* m_audioMetadata;
	PcmFrameQueue* m_frameQueue;

	CcpThread m_submitThread;
	uint64_t m_length;
	float m_volume;
	bool m_stopRequested;
	bool m_finishedSubmitting;
	uint32_t m_submittedSamples;
	Timer m_timer;

	static const uint32_t BUFFER_SIZE = 16384;
	float m_buffers[2][16384];
	uint32_t m_bufferSizes[2];
	CcpAtomic<uint32_t> m_bufferReady;
	uint32_t m_paused;
	unsigned int m_startSampleCount;
	bool m_started;
};

TYPEDEF_BLUECLASS( Audio2Sink );


#endif