////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "WaveOutAudioSink.h"

#ifdef _WIN32

WaveOutAudioSink::WaveOutAudioSink()
	:m_waveOut( nullptr ),
	m_audioMetadata( nullptr ),
	m_frameQueue( nullptr ),
	m_done( false ),
	m_pauseCounter( 0 ),
	m_stopRequested( false ),
	m_submitThread( nullptr ),
	m_waveOutMutex( "WaveOutAudioSink", "m_waveOutMutex" ),
	m_stoppedTime( 0 )
{
	memset( m_headerRing, 0, sizeof( m_headerRing ) );
}

WaveOutAudioSink::~WaveOutAudioSink()
{
	Close();
}

void WaveOutAudioSink::Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue )
{
	m_audioMetadata = &audioMetadata;
	m_frameQueue = &frameQueue;
	m_submitThread = CreateThread( nullptr, 0, &SubmitThreadHelper, this, 0, nullptr );
}

void WaveOutAudioSink::Close()
{
	if( m_submitThread )
	{
		m_stopRequested = true;
		WaitForSingleObject( m_submitThread, INFINITE );
		CloseHandle( m_submitThread );
		m_submitThread = nullptr;
	}
	m_audioMetadata = nullptr;
	m_frameQueue = nullptr;
}

void WaveOutAudioSink::Pause()
{
	CcpAutoMutex lock( m_waveOutMutex );
	if( m_waveOut && m_pauseCounter == 0 )
	{
		waveOutPause( m_waveOut );
	}
	++m_pauseCounter;
}

void WaveOutAudioSink::Resume()
{
	CcpAutoMutex lock( m_waveOutMutex );
	--m_pauseCounter;
	if( m_waveOut && m_pauseCounter == 0 )
	{
		waveOutRestart( m_waveOut );
	}
}

uint64_t WaveOutAudioSink::GetTime()
{
	CcpAutoMutex lock( m_waveOutMutex );

	if( !m_waveOut )
	{
		return m_stoppedTime;
	}
	MMTIME mt;
	mt.wType = TIME_SAMPLES;
	mt.u.sample = 0;
	waveOutGetPosition( m_waveOut, &mt, sizeof( mt ) );
	return ( uint64_t( mt.u.sample ) * 1000000 ) / uint64_t( m_audioMetadata->rate ) * 1000;
}

bool WaveOutAudioSink::IsDone()
{
	return m_done;
}

void WaveOutAudioSink::FinishPlaying()
{
	while( !m_stopRequested )
	{
		bool isPlaying = false;
		for( int i = 0; i < MAX_QUEUED_BUFFERS; ++i )
		{
			if( ( m_headerRing[i].dwFlags & WHDR_PREPARED ) && ( m_headerRing[i].dwFlags & WHDR_DONE ) == 0 )
			{
				isPlaying = true;
				break;
			}
		}
		if( !isPlaying )
		{
			m_done = true;
			break;
		}
		Sleep( 10 );
	}
}

bool WaveOutAudioSink::CreateDevice()
{
	WAVEFORMATEX wfx;
	wfx.nSamplesPerSec = m_audioMetadata->rate;
	wfx.wBitsPerSample = 16;
	wfx.nChannels = m_audioMetadata->channels;
	wfx.cbSize = 0; 
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = ( wfx.wBitsPerSample >> 3 ) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	CcpAutoMutex lock( m_waveOutMutex );

	if( waveOutOpen( &m_waveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL ) != MMSYSERR_NOERROR ) 
	{
		m_waveOut = nullptr;
		return false;
	}
	if( m_pauseCounter )
	{
		waveOutPause( m_waveOut );
	}
	return true;
}

void WaveOutAudioSink::SubmitThread()
{
	std::unique_ptr<PcmFrame, TrackableDelete<PcmFrame>> frameRing[MAX_QUEUED_BUFFERS];
	uint32_t headerIndex = 0;

	while( !m_stopRequested )
	{
		std::unique_ptr<PcmFrame, TrackableDelete<PcmFrame>> packet( m_frameQueue->Pop() );
		if( !packet )
		{
			FinishPlaying();
			break;
		}
		
		MMRESULT result;
		if( !m_waveOut )
		{
			if( !CreateDevice() )
			{
				break;
			}
		}
		if( packet->samples == 0 )
		{
			continue;
		}

		WAVEHDR& header = m_headerRing[headerIndex];

		if( header.dwFlags & WHDR_PREPARED )
		{
			while(!m_stopRequested && waveOutUnprepareHeader( m_waveOut, &header, sizeof(WAVEHDR) ) == WAVERR_STILLPLAYING)
			{
				Sleep(1);
			}
		}
		ZeroMemory(&header, sizeof(WAVEHDR));
		header.dwBufferLength = packet->samples * packet->channels * 2;
		header.lpData = reinterpret_cast<char*>( packet->data.get() );

		if( waveOutPrepareHeader(m_waveOut, &header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR )
		{
			result = waveOutWrite(m_waveOut, &header, sizeof(WAVEHDR));
			headerIndex = ( headerIndex + 1 ) % MAX_QUEUED_BUFFERS;
			frameRing[headerIndex] = std::move( packet );
		}
	}
	if( m_waveOut )
	{
		CcpAutoMutex lock( m_waveOutMutex );

		MMTIME mt;
		mt.wType = TIME_SAMPLES;
		mt.u.sample = 0;
		waveOutGetPosition( m_waveOut, &mt, sizeof( mt ) );
		m_stoppedTime = ( uint64_t( mt.u.sample ) * 1000000 ) / uint64_t( m_audioMetadata->rate ) * 1000;

		waveOutReset( m_waveOut );
		for( int i = 0; i < MAX_QUEUED_BUFFERS; ++i )
		{
			if( m_headerRing[i].dwFlags & WHDR_PREPARED )
			{
				waveOutUnprepareHeader( m_waveOut, &m_headerRing[i], sizeof(WAVEHDR) );
			}
		}
		waveOutClose( m_waveOut );
		m_waveOut = nullptr;
	}
}

DWORD WINAPI WaveOutAudioSink::SubmitThreadHelper( LPVOID lpThreadParameter )
{
	static_cast<WaveOutAudioSink*>( lpThreadParameter )->SubmitThread();
	return 0;
}

#endif