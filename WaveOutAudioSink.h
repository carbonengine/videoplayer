////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef WaveOutAudioSink_H
#define WaveOutAudioSink_H

#include "IAudioSinkExposed.h"

#ifdef _WIN32

// --------------------------------------------------------------------------------------
// Description:
//   A simple audio sink/player that uses Windows wave audio API. It is not cross-
//   platform and should only be used for tests.
// See also:
//   IAudioSinkExposed
// --------------------------------------------------------------------------------------
BLUE_CLASS( WaveOutAudioSink ): 
	public IAudioSinkExposed
{
public:
	WaveOutAudioSink();
	~WaveOutAudioSink();

	EXPOSE_TO_BLUE();

	virtual void Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue );
	virtual void Close();
	virtual void Pause();
	virtual void Resume();
	virtual uint64_t GetTime();
	virtual bool IsDone();
private:
	void SubmitThread();
	static DWORD WINAPI SubmitThreadHelper( LPVOID lpThreadParameter );
	void FinishPlaying();
	bool CreateDevice();

	static const size_t MAX_QUEUED_BUFFERS = 10;
	HWAVEOUT m_waveOut;
	WAVEHDR m_headerRing[MAX_QUEUED_BUFFERS];
	const AudioMetadata* m_audioMetadata;
	PcmFrameQueue* m_frameQueue;
	uint32_t m_pauseCounter;
	bool m_done;
	bool m_stopRequested;
	HANDLE m_submitThread;
	CcpMutex m_waveOutMutex;
	uint64_t m_stoppedTime;
	uint64_t m_timeOffset;
};

TYPEDEF_BLUECLASS( WaveOutAudioSink );

#endif

#endif