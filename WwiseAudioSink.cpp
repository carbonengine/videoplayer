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
	m_pauseCounter( 0 ),
	m_samplesSubmitted( 0 ),
	m_samplesPlayed( 0 ),
	m_volume( 1 ),
	m_playing( false )
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
// bufferData is a pointer to the buffer Wwise wants filled.
// Returns the number of valid sample frames successfully copied to Wwises buffer.
int WwiseAudioSink::FillBuffer( IAudioInputMgr::BufferData& bufferData )
{
	if( m_pauseCounter > 0 )
	{
		return 0;
	}
	else
	{
		m_timer.Start();
		int samplesProcessed = 0;
		bool fillBuffer = true;
		int16_t* wwiseBuffer = bufferData.data;
		while( fillBuffer )
		{
			auto packetPeek = m_frameQueue->Peek();

			// If there are no more packets then the video is finished playing.
			if( !packetPeek )
			{
				if( m_frameQueue->IsComplete() )
				{
					FinishPlaying();
				}
				return 0;
			}

			// Videoplayer signals it is seeking by sending a single packet with 0 samples.
			if( packetPeek->samples == 0 )
			{
				auto packet = m_frameQueue->Pop();
				m_samplesSubmitted = packet->timeStamp * m_audioMetadata->rate / NSEC_TO_SEC;
				m_samplesPlayed = m_samplesSubmitted;

				return 0;
			}

			int samplesToProcess = packetPeek->samples * packetPeek->channels;
			if( samplesProcessed + samplesToProcess <= bufferData.numSamples )
			{
				auto packet = m_frameQueue->Pop();
				memcpy( wwiseBuffer, packetPeek->data, samplesToProcess * m_audioMetadata->bps / 8 ); // A sample is 16 bytes
				wwiseBuffer += samplesToProcess * m_audioMetadata->bps / 16; // Move pointer forward so we don't keep putting samples in head of the buffer.
				samplesProcessed += samplesToProcess;
			}
			else
			{
				samplesToProcess = bufferData.numSamples - samplesProcessed; // Make sure we don't go above the number of samples Wwises buffer can handle.
				memcpy( wwiseBuffer, packetPeek->data, samplesToProcess * m_audioMetadata->bps / 8 );
				wwiseBuffer += samplesToProcess * m_audioMetadata->bps / 16; 
				// Move pointer in the packet since we have processed a portion of its samples now. 
				// We don't pop this packet from the frame queue yet because we have only processed a portion of it.
				packetPeek->samples -= samplesToProcess / packetPeek->channels;
				packetPeek->data += samplesToProcess * m_audioMetadata->bps / 16;

				samplesProcessed += samplesToProcess;
				fillBuffer = false;
			}
		}

		m_samplesPlayed = m_samplesSubmitted;
		m_samplesSubmitted += samplesProcessed / bufferData.numChannels;
		return samplesProcessed / bufferData.numChannels;
	}
}

void WwiseAudioSink::Open( const AudioMetadata& audioMetadata, PcmFrameQueue& frameQueue )
{
	m_audioMetadata = &audioMetadata;
	m_frameQueue = &frameQueue;
	if( !m_playing )
	{
		m_audioInputMgr->StartInput( audioMetadata.channels, audioMetadata.bps, audioMetadata.rate );
		m_audioInputMgr->SetVolume( m_volume );
		m_playing = true;
	}
}

void WwiseAudioSink::Close()
{
	m_stopRequested = true;

	m_audioInputMgr->StopInput();

	m_audioMetadata = nullptr;
	m_frameQueue = nullptr;
	m_playing = false;
}

void WwiseAudioSink::Pause()
{
	m_pauseCounter += 1;
}

void WwiseAudioSink::Resume()
{
	m_pauseCounter -= 1;
}

// Note: the video player relies on its audio sink to know where it is in the video.
// This function is supposed to return how many seconds have passed in the playing video.
uint64_t WwiseAudioSink::GetTime()
{
	if( m_audioMetadata != nullptr )
	{
		// The number of samples played needs to be smoothed to avoid micro jitters, hence the use of m_timer.GetTime().
		// However, it should never be smoothed more than the number of samples submitted to Wwise.
		return std::min( ( m_timer.GetTime() + ( m_samplesPlayed * NSEC_TO_SEC ) / m_audioMetadata->rate ), ( ( m_samplesSubmitted * NSEC_TO_SEC ) / m_audioMetadata->rate ) );
	}

	return 0;
}

bool WwiseAudioSink::IsDone()
{
	return m_done || m_stopRequested;
}

void WwiseAudioSink::FinishPlaying()
{
	m_done = true;
	m_audioInputMgr->StopInput();
}

void WwiseAudioSink::SetVolume( float volume )
{
	// Volume must be between 0 and 1
	if( volume < 0 )
	{
		m_volume = 0;
	}
	else if( volume > 1 )
	{
		m_volume = 1;
	}
	else
	{
		m_volume = volume;
	}

	if( m_audioInputMgr != nullptr )
	{
		m_audioInputMgr->SetVolume( m_volume );
	}
}

float WwiseAudioSink::GetVolume()
{
	return m_volume;
}
