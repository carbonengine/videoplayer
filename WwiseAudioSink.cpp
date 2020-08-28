////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Eric Nielsen
// Created:		August 2020
// Copyright:	CCP 2020
//

#include "StdAfx.h"
#include "WwiseAudioSink.h"

using namespace std::placeholders;


WwiseAudioSink::WwiseAudioSink() :
	m_audioMetadata( nullptr ),
	m_frameQueue( nullptr ),
	m_done( false ),
	m_stopRequested( false ),
	m_submitThread( nullptr ),
	m_bufferQueueMutex( "WwiseAudioSink", "m_bufferQueueMutex" ),
	m_timeOffset( 0 ),
	m_playing( false ),
	m_samplesSubmitted( 0 )
{
}

WwiseAudioSink::~WwiseAudioSink()
{
	Close();
}

// Create an instance of WwiseAudioSink. An instance of audio2's AudioInputMgr
// must be created and passed in from Python.
void WwiseAudioSink::Create( IAudioInputMgr* inputMgr )
{
	m_audioInputMgr = inputMgr;
	inputMgr->SetSink( this );
}

// A callback for the audio input manager that will be called every audio frame in Wwise.
// bufferData includes the buffer to fill for that Wwise plays back.
// Returns the number of valid sample frames successfully copied to Wwises buffer.
int WwiseAudioSink::FillBuffer( IAudioInputMgr::BufferData& bufferData )
{

	if( m_playing )
	{
		CcpAutoMutex lock( m_bufferQueueMutex );
		// Fill Wwises buffer with as much as it can handle from what our internal buffer has built up.
		auto numSamples = std::min( bufferData.numSamples, (int)( m_threadBuffer.size() ) );
		std::copy( m_threadBuffer.begin(), m_threadBuffer.begin() + numSamples, bufferData.data );
		m_threadBuffer.erase( m_threadBuffer.begin(), m_threadBuffer.begin() + numSamples );

		m_samplesSubmitted += numSamples / bufferData.numChannels;

		return numSamples / bufferData.numChannels;
	}

	return 0;
}

void WwiseAudioSink::Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue )
{
	m_audioMetadata = &audioMetadata;
	m_frameQueue = &frameQueue;
	m_audioInputMgr->StartInput( audioMetadata.channels, audioMetadata.bps, audioMetadata.rate );
	if( !m_submitThread )
	{
		m_submitThread = CreateThread( nullptr, 0, &SubmitThreadHelper, this, 0, nullptr );
	}
}

void WwiseAudioSink::Close()
{
	if( m_submitThread )
	{
		m_stopRequested = true;
		WaitForSingleObject( m_submitThread, INFINITE );
		CloseHandle( m_submitThread );
		m_submitThread = nullptr;
	}

	m_audioInputMgr->StopInput();

	m_audioMetadata = nullptr;
	m_frameQueue = nullptr;
	m_playing = false;
}

void WwiseAudioSink::Pause()
{
	m_playing = false;
	if( m_timer.IsRunning() )
	{
		m_timer.Pause();
	}
}

void WwiseAudioSink::Resume()
{
	m_playing = true;
	if( !m_timer.IsRunning() )
	{
		m_timer.Start();
	}
	else if( m_timer.IsPaused() )
	{
		m_timer.Resume();
	}
}

uint64_t WwiseAudioSink::GetTime()
{
	if( !m_playing )
	{
		return m_timeOffset;
	}

	return m_timeOffset + m_timer.GetTime();
}

bool WwiseAudioSink::IsDone()
{
	if( m_stopRequested) 
	{
		return true;
	}

	uint64_t currentTime = GetTime();
	uint64_t videoLength = m_samplesSubmitted * 100000000 / m_audioMetadata->rate; 
	return currentTime > videoLength;
}

void WwiseAudioSink::SubmitThread()
{
	while( !m_stopRequested )
	{
		auto packet = m_frameQueue->Pop();
		if( !packet )
		{
			break;
		}
		if( packet->samples == 0 )
		{
			m_timeOffset = packet->timeStamp;
		}
		CcpAutoMutex lock( m_bufferQueueMutex );
		auto sampleCount = packet->samples * packet->channels;
		m_threadBuffer.insert( m_threadBuffer.end(), packet->data, packet->data + sampleCount );
	}
}

DWORD WINAPI WwiseAudioSink::SubmitThreadHelper( LPVOID lpThreadParameter )
{
	static_cast<WwiseAudioSink*>( lpThreadParameter )->SubmitThread();
	return 0;
}
