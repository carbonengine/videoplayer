////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "VideoController.h"

CCP_STATS_DECLARE( videoplayerInitialBufferingTime, "VideoPlayer/initialBuffering", false, CST_COUNTER_HIGH, "total initial buffering time in MS" );
CCP_STATS_DECLARE( videoplayerBufferingTime, "VideoPlayer/buffering", false, CST_COUNTER_HIGH, "total during-playback buffering time in MS" );
CCP_STATS_DECLARE( videoplayerDestructor, "VideoPlayer/destructorTime", false, CST_TIME, "time to destroy a controller" );

namespace
{

uint64_t AFTER_BUFFERING_TIME = 2 * 1000000000;
uint64_t DROP_FRAME_THRESHOLD = 50000000;

}


VideoController::VideoController( ICcpStream* stream, IAudioSink* audioSink, unsigned audioTrack, bool looped )
	:m_state( UNINITIALIZED ),
	m_paused( false ),
	m_audioSink( audioSink ),
	m_seekOffset( 0 )
{
	if( m_audioSink )
	{
		m_audioSink->Pause();
	}
	m_mediaTime.Pause();

	m_parser.reset( CreateVideoContainerParser( stream, audioSink ? STREAM_AUDIO_VIDEO : STREAM_VIDEO, audioTrack, looped ) );
	m_state = PARSING_METADATA;
}

VideoController::~VideoController()
{
#ifndef WITH_TESTS
	CCP_STATS_SCOPED_TIME( videoplayerDestructor );
#endif

	if( m_audioDecoder )
	{
		m_audioDecoder->GetDecodedQueue().SetComplete();
	}
	if( m_videoDecoder )
	{
		m_videoDecoder->GetDecodedQueue().SetComplete();
	}
	if( m_audioSink )
	{
		m_audioSink->Close();
	}
	m_parser->CompleteQueues();
	m_audioDecoder.reset();
	m_videoDecoder.reset();
	m_parser.reset();
}

void VideoController::GetErrors( Errors& errors )
{
	errors.parserError = m_parser ? m_parser->GetError() : PARSER_ERROR_OK;
	errors.audioDecoderError = m_audioDecoder ? m_audioDecoder->GetError() : DECODER_ERROR_OK;
	errors.videoDecoderError = m_videoDecoder ? m_videoDecoder->GetError() : DECODER_ERROR_OK;
}

void VideoController::Pause()
{
	if( m_paused )
	{
		return;
	}
	m_paused = true;
	if( m_audioSink )
	{
		m_audioSink->Pause();
	}
	m_mediaTime.Pause();
}

void VideoController::Resume()
{
	if( !m_paused )
	{
		return;
	}
	m_paused = false;
	if( m_audioSink )
	{
		m_audioSink->Resume();
	}
	m_mediaTime.Resume();
}

void VideoController::Seek( uint64_t time )
{
	if( m_state <= INITIAL_BUFFERING || m_state > FINISHING_BUFFERING )
	{
		return;
	}
	if( m_audioSink )
	{
		m_audioSink->Pause();
	}
	m_mediaTime.Pause();

	m_seekOffset = time;
	m_parser->Seek( time );
	m_state = INITIAL_BUFFERING;
	if( m_audioDecoder )
	{
		m_audioDecoder->GetDecodedQueue().Clear();
	}
	if( m_videoDecoder )
	{
		m_videoDecoder->GetDecodedQueue().Clear();
	}
}

bool VideoController::IsPaused() const
{
	return m_paused;
}

IVideoContainerParser& VideoController::GetParser()
{
	return *m_parser;
}

void VideoController::Update()
{
	switch( m_state )
	{
	case UNINITIALIZED:
	case DONE:
		return;
	case PARSING_METADATA:
		if( m_parser->GetError() != PARSER_ERROR_OK )
		{
			m_state = DONE;
			return;
		}
		if( !m_parser->IsMetadataAvailable() )
		{
			return;
		}
		if( m_parser->GetVideoQueue() )
		{
			m_videoDecoder.reset( CreateVideoDecoder( m_parser->GetVideoMetadata(), *m_parser->GetVideoQueue() ) );
		}
		if( m_parser->GetAudioQueue() )
		{
			m_audioDecoder.reset( CreateAudioDecoder( m_parser->GetAudioMetadata(), *m_parser->GetAudioQueue() ) );
		}
		else
		{
			m_audioSink = nullptr;
		}
		m_state = INITIAL_BUFFERING;
		m_bufferingTimer.Start();
	case INITIAL_BUFFERING:
		if( ( m_audioDecoder && !m_audioDecoder->GetDecodedQueue().IsFull() ) ||
			( m_videoDecoder && !m_videoDecoder->GetDecodedQueue().IsFull() ) ||
			!( ( m_parser->GetAudioQueue() && m_parser->GetAudioQueue()->IsFull() ) ||
			( m_parser->GetVideoQueue() && m_parser->GetVideoQueue()->IsFull() ) ) )
		{
			return;
		}
		CCP_STATS_ADD( videoplayerInitialBufferingTime, int32_t( m_bufferingTimer.GetTime() / 1000000 ) );
		m_state = PLAYING;
		if( m_audioSink )
		{
			m_audioSink->Open( m_parser->GetAudioMetadata(), m_audioDecoder->GetDecodedQueue() );
		}
		m_mediaTime.Start();

		if( m_audioSink )
		{
			m_audioSink->Resume();
		}
		m_mediaTime.Resume();

		Update();
		break;
	case BUFFERING:
		RemoveExpiredFrames();
		if( NeedsBuffering() )
		{
			return;
		}
		m_state = FINISHING_BUFFERING;
		CCP_STATS_ADD( videoplayerBufferingTime, int32_t( m_bufferingTimer.GetTime() / 1000000 ) );
		m_bufferingTimer.Start();
		CCP_LOG( "VideoController: starting buffering followup pause" );
	case FINISHING_BUFFERING:
		RemoveExpiredFrames();
		if( m_bufferingTimer.GetTime() < AFTER_BUFFERING_TIME )
		{
			return;
		}
		m_state = PLAYING;
		if( m_audioSink )
		{
			m_audioSink->Resume();
		}
		m_mediaTime.Resume();

		CCP_LOG( "VideoController: resuming playback after buffering" );
	case PLAYING:
		if( m_paused )
		{
			return;
		}
		if( m_videoDecoder && m_audioDecoder )
		{
			m_videoDecoder->SetDropFrameTime( GetMediaTime() - DROP_FRAME_THRESHOLD );
		}
		RemoveExpiredFrames();
		if( IsDone() )
		{
			m_state = DONE;
			CCP_LOG( "VideoController: playback is finished" );
		}
		else if( NeedsBuffering() )
		{
			CCP_LOG( "VideoController: starting buffering because audio/video queues are running low on frames" );
			m_state = BUFFERING;
			if( m_audioSink )
			{
				m_audioSink->Pause();
			}
			m_mediaTime.Pause();

			m_bufferingTimer.Start();
		}
	}
}

VideoFrame* VideoController::GetVideoFrame()
{
	if( m_state < PLAYING )
	{
		return nullptr;
	}
	return m_videoDecoder->GetDecodedQueue().Peek();
}

uint64_t VideoController::GetMediaTime()
{
	if( m_state == INITIAL_BUFFERING )
	{
		return m_seekOffset;
	}
	return m_audioSink ? m_audioSink->GetTime() : m_seekOffset + m_mediaTime.GetTime();
}

void VideoController::RemoveExpiredFrames()
{
	if( m_videoDecoder )
	{
		uint64_t mediaTime = GetMediaTime();
		while( true )
		{
			auto frame = m_videoDecoder->GetDecodedQueue().Peek();
			auto nextFrame = m_videoDecoder->GetDecodedQueue().Peek( 1 );
			if( !frame )
			{
				break;
			}
			if( frame->timeStamp > mediaTime || ( ( !nextFrame || nextFrame->timeStamp > mediaTime ) && !m_videoDecoder->GetDecodedQueue().IsComplete() ) )
			{
				break;
			}
			m_videoDecoder->GetDecodedQueue().Pop();
		}
	}
}

bool VideoController::NeedsBuffering() const
{
	return ( m_videoDecoder && !m_videoDecoder->GetDecodedQueue().IsComplete() && m_videoDecoder->GetDecodedQueue().Size() == 0 ) ||
		( m_audioDecoder && !m_audioDecoder->GetDecodedQueue().IsComplete() && m_audioDecoder->GetDecodedQueue().Size() < 10 );
}

bool VideoController::IsDone() const
{
	if( m_videoDecoder )
	{
		auto frame = m_videoDecoder->GetDecodedQueue().Peek();
		return !frame && m_videoDecoder->GetDecodedQueue().IsComplete() && ( !m_audioSink || m_audioSink->IsDone() );
	}
	else if( m_audioSink )
	{
		return m_audioSink->IsDone();
	}
	else
	{
		return true;
	}
}
