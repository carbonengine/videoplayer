////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "Audio2Sink.h"

Audio2Sink::Audio2Sink()
	:m_setAudioStream( nullptr ),
	m_outputChannel( 0 ),
	m_audioMetadata( nullptr ),
	m_frameQueue( nullptr ),
	m_length( 0 ),
	m_stopRequested( false ),
	m_finishedSubmitting( false ),
	m_bufferReady( 2 )
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

void Audio2Sink::Create( intptr_t audioFunction, int32_t outputChannel )
{
	m_setAudioStream = reinterpret_cast<SetAudioStream>( audioFunction );
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
	m_timer.Pause();
}

void Audio2Sink::Resume()
{
	m_timer.Resume();
}

uint64_t Audio2Sink::GetTime()
{
	return m_timer.GetTime();
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
	auto t = m_timer.GetTime();
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
			if( !m_timer.IsRunning() )
			{
				m_timer.Start();
			}
			( *m_setAudioStream )( m_buffers[m_bufferReady], m_bufferSizes[m_bufferReady], m_outputChannel );
			m_bufferReady = 3;
		}
	}
}

void Audio2Sink::SubmitThread()
{
	float buffer[16384];
	float volume = 1.f;
	bool firstFrame = true;
	uint32_t currentBuffer = 0;

	while( !m_stopRequested )
	{
		std::unique_ptr<PcmFrame, TrackableDelete<PcmFrame>> packet( m_frameQueue->Pop() );
		if( !packet )
		{
			m_bufferReady = currentBuffer;
			m_finishedSubmitting = true;
			break;
		}

		if( packet->channels == 1 )
		{
			for( unsigned i = 0; i < packet->samples; ++i )
			{
				buffer[i * 2] =  ( (float) packet->data[i] ) * volume / 0x8000;
				buffer[i * 2 + 1] =  buffer[i * 2];
			}
		}
		else
		{
			for( unsigned i = 0; i < packet->samples * packet->channels; ++i )
			{
				buffer[i] =  ( (float) packet->data[i] ) * volume / 0x8000;
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
