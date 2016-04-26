////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "Audio2Sink.h"

// samples per millisecond
static const uint32_t WWISE_RATE = 48;

Audio2Sink::Audio2Sink()
	:m_setAudioStream( nullptr ),
	m_outputChannel( 0 ),
	m_audioMetadata( nullptr ),
	m_frameQueue( nullptr ),
	m_length( 0 ),
	m_stopRequested( false ),
	m_finishedSubmitting( false ),
	m_bufferReady( 2 ),
	m_volume( 1.f ),
	m_submittedSamples( 0 ),
	m_paused( 0 ),
	m_startSampleCount( -1 ),
	m_started( false )
{
	m_bufferSizes[0] = 0;
	m_bufferSizes[1] = 0;
#ifndef WITH_TESTS
	BeOS->RegisterForTicks( this, nullptr );
#endif
}

Audio2Sink::~Audio2Sink()
{
#ifndef WITH_TESTS
	BeOS->UnregisterForTicks( this, nullptr );
#endif
	Close();
}

void Audio2Sink::Create( intptr_t audioFunction, intptr_t audioPositionFunction, int32_t outputChannel )
{
	m_setAudioStream = reinterpret_cast<SetAudioStream>( audioFunction );
	m_getAudioStreamPosition = reinterpret_cast<GetAudioStreamPosition>( audioPositionFunction );
	m_outputChannel = outputChannel;
}

void Audio2Sink::Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue )
{
	if( m_submitThread.joinable() )
	{
		Close();
	}
	m_audioMetadata = &audioMetadata;
	m_frameQueue = &frameQueue;
	m_submitThread = CcpThread( &Audio2Sink::SubmitThread, this );
}

void Audio2Sink::Close()
{
	m_stopRequested = true;
	m_submitThread.join();
	m_audioMetadata = nullptr;
	m_frameQueue = nullptr;
}

void Audio2Sink::Pause()
{
	++m_paused;
}

void Audio2Sink::Resume()
{
	--m_paused;
}

uint64_t Audio2Sink::GetTime()
{
	if( !m_started )
	{
		return 0;
	}
	auto sampleCount = uint64_t( ( *m_getAudioStreamPosition )( m_outputChannel ) );
	if( sampleCount < m_startSampleCount )
	{
		sampleCount += unsigned( -1 );
	}
	sampleCount -= m_startSampleCount;
	if( m_finishedSubmitting && sampleCount >= m_submittedSamples )
	{
		if( !m_timer.IsRunning() )
		{
			m_timer.Start();
		}
	}
	return sampleCount / 2 * 1000000 / WWISE_RATE + m_timer.GetTime();
}

bool Audio2Sink::IsDone()
{
	if( !m_finishedSubmitting )
	{
		return false;
	}
	if( !m_audioMetadata )
	{
		return false;
	}
	auto t = GetTime();
	auto length = m_length * 100000000 / m_audioMetadata->rate;
	return t > length;
}

void Audio2Sink::OnTick( Be::Time realTime, Be::Time simTime, void* cookie )
{
	if( m_bufferReady < 2 )
	{
		auto available = ( *m_setAudioStream )( nullptr, 0, m_outputChannel );
		if( available >= m_bufferSizes[m_bufferReady] )
		{
			if( !m_paused )
			{
				if( !m_started )
				{
					m_started = true;
					m_startSampleCount = ( *m_getAudioStreamPosition )( m_outputChannel );
				}
				m_submittedSamples += m_bufferSizes[m_bufferReady] / 2;
				( *m_setAudioStream )( m_buffers[m_bufferReady], m_bufferSizes[m_bufferReady], m_outputChannel );
				m_bufferReady = 3;
			}
		}
	}
}

void Audio2Sink::SubmitThread()
{
	float buffer[16384];
	bool firstFrame = true;
	uint32_t currentBuffer = 0;

	while( !m_stopRequested )
	{
		auto packet = m_frameQueue->Pop();
		if( !packet )
		{
			m_bufferReady = currentBuffer;
			m_finishedSubmitting = true;
			break;
		}

		if( m_volume == 0 )
		{
			if( packet->channels == 1 )
			{
				for( unsigned i = 0; i < packet->samples; ++i )
				{
					buffer[i * 2] =  0;
					buffer[i * 2 + 1] =  0;
				}
			}
			else
			{
				for( unsigned i = 0; i < packet->samples * packet->channels; ++i )
				{
					buffer[i] =  0;
				}
			}
		}
		else
		{
			if( packet->channels == 1 )
			{
				for( unsigned i = 0; i < packet->samples; ++i )
				{
					buffer[i * 2] =  ( (float) packet->data[i] ) * m_volume / 0x8000;
					buffer[i * 2 + 1] =  buffer[i * 2];
				}
			}
			else
			{
				for( unsigned i = 0; i < packet->samples * packet->channels; ++i )
				{
					buffer[i] =  ( (float) packet->data[i] ) * m_volume / 0x8000;
				}
			}
		}

		auto samples = packet->samples * 2;
		auto ptr = buffer;
		while( samples )
		{
			auto availableSize = BUFFER_SIZE - m_bufferSizes[currentBuffer];
			if( availableSize == 0 )
			{
				while( m_bufferReady < 2 )
				{
					if( m_stopRequested )
					{
						return;
					}
					CcpThreadSleep( 1 );
				}
				m_bufferReady = currentBuffer;
				currentBuffer = 1 - currentBuffer;
				m_bufferSizes[currentBuffer] = 0;
			}
			auto submit = std::min( samples, availableSize );
			memcpy( m_buffers[currentBuffer] + m_bufferSizes[currentBuffer], ptr, submit * sizeof( float ) );
			m_bufferSizes[currentBuffer] += submit;
			ptr += submit;
			samples -= submit;
		}
		m_length += packet->samples;
	}
}
