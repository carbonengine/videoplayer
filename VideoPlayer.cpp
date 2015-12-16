////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "VideoPlayer.h"
#include "VideoController.h"
#include "IAudioSinkExposed.h"

#ifndef WITH_TESTS

namespace
{

// --------------------------------------------------------------------------------------
// Description:
//   A specual deleter class for VideoPlayer. Since deleting a still-playing video can
//   be time consuming (for example if the parser is stuck in a stream reading function)
//   we delete the controller on a separate thread and clean up the stream and audio sink
//   on the main thread afterwards.
// See also:
//   VideoPlayer
// --------------------------------------------------------------------------------------
class VideoPlayerDeleter: public IBlueEvents
{
public:
	static void DeletePlayer( VideoController* video, IBlueStream* stream, IAudioSinkExposed* audioSink )
	{
		auto data = CCP_NEW( "PlayerData" ) PlayerData;
		data->m_video.reset( video );
		data->m_stream = stream;
		data->m_audioSink = audioSink;
		data->m_deleted = 0;
		data->m_thread = CcpThread( &PlayerData::DeleteMe, data );
		
		static VideoPlayerDeleter s_deleter;
		s_deleter.m_players.push_back( data );
	}
protected:
	virtual void OnTick( Be::Time, Be::Time, void* )
	{
		for( size_t i = 0; i < m_players.size(); )
		{
			if( m_players[i]->m_deleted )
			{
				m_players[i]->m_thread.join();
				CCP_DELETE m_players[i];
				m_players.erase( m_players.begin() + i );
			}
			else
			{
				++i;
			}
		}
	}
private:
	VideoPlayerDeleter()
		:m_players( "VideoPlayerDeleter::m_players" )
	{
		BeOS->RegisterForTicks( this, nullptr );
	}
	struct PlayerData
	{
		void DeleteMe()
		{
			m_video.reset();
			m_deleted = 1;
		}
		std::unique_ptr<VideoController, TrackableDelete<VideoController>> m_video;
		IBlueStreamPtr m_stream;
		IAudioSinkExposedPtr m_audioSink;
		CcpAtomic<uint32_t> m_deleted;
		CcpThread m_thread;
	};
	TrackableStdVector<PlayerData*> m_players;
};

}
#endif

VideoPlayer::VideoPlayer(  IRoot* lockobj )
	:m_lastUpdatedTimeStamp( std::numeric_limits<uint64_t>::max() ),
	m_lastState( VideoController::State( -1 ) )
{
#ifndef WITH_TESTS
	BeOS->RegisterForTicks( this, nullptr );
#endif
}

VideoPlayer::~VideoPlayer()
{
#ifndef WITH_TESTS
	BeOS->UnregisterForTicks( this, nullptr );

	VideoPlayerDeleter::DeletePlayer( m_video.release(), m_stream, m_audioSink );
#else
	m_video.reset();
#endif
}

bool VideoPlayer::OnModified( Be::Var* value )
{
	m_lastUpdatedTimeStamp = std::numeric_limits<uint64_t>::max();
	return true;
}

Be::BlueStdResult VideoPlayer::Create( IBlueStream* stream, IAudioSinkExposed* audioSink, unsigned audioTrack )
{
	if( !stream )
	{
		return Be::BlueStdResult( Be::BLUE_STD_RESULT_VALUE_ERROR, "need a valid stream" );
	}
	m_video.reset( CCP_NEW( "VideoPlayer.m_video" ) VideoController( stream, audioSink, audioTrack ) );
	m_stream = stream;
	m_audioSink = audioSink;
	return Be::BlueStdResult( Be::BLUE_STD_RESULT_OK );
}

uint64_t VideoPlayer::GetMediaTime() const
{
	return m_video ? m_video->GetMediaTime() : 0u;
}

uint64_t VideoPlayer::GetDuration() const
{
	return m_video ? m_video->GetParser().GetDuration() : 0u;
}

uint64_t VideoPlayer::GetDownloadedMediaTime() const
{
	return m_video ? m_video->GetParser().GetDownloadedMediaTime() : 0u;
}

VideoPlayerResult VideoPlayer::Update()
{
	if( !m_video )
	{
		return "video player is not initialized";
	}
	m_video->Update();
	VideoController::Errors errors;
	m_video->GetErrors( errors );
	if( !errors )
	{
		if( m_lastError != errors )
		{
			m_lastError = errors;
			if( m_onError )
			{
				m_onError.CallVoid( this );
			}
		}
		return errors;
	}
	auto state = m_video->GetState();
	if( m_lastState != state )
	{
		if( m_onStateChange )
		{
			m_onStateChange.CallVoid( this );
		}
		m_lastState = state;
	}
	if( m_yChannel || m_uChannel || m_vChannel )
	{
		auto frame = m_video->GetVideoFrame();
		if( frame && m_lastUpdatedTimeStamp != frame->timeStamp )
		{
			if( ( m_yChannel && ( m_yChannel->GetWidth() != frame->yWidth || m_yChannel->GetHeight() != frame->yHeight ) ) ||
				( m_uChannel && ( m_uChannel->GetWidth() != frame->uvWidth || m_uChannel->GetHeight() != frame->uvHeight ) ) ||
				( m_vChannel && ( m_vChannel->GetWidth() != frame->uvWidth || m_vChannel->GetHeight() != frame->uvHeight ) ) )
			{
				if( m_onCreateTextures )
				{
					m_onCreateTextures.CallVoid( this, std::make_pair( frame->yWidth, frame->yHeight ), std::make_pair( frame->uvWidth, frame->uvHeight ) );
				}
			}

			if( m_yChannel && m_yChannel->GetWidth() == frame->yWidth && m_yChannel->GetHeight() == frame->yHeight )
			{
				m_yChannel->UpdateSubresource( 0, 0, frame->yWidth, frame->yHeight, frame->y.get(), frame->yWidth );
			}
			if( m_uChannel && m_uChannel->GetWidth() == frame->uvWidth && m_uChannel->GetHeight() == frame->uvHeight )
			{
				m_uChannel->UpdateSubresource( 0, 0, frame->uvWidth, frame->uvHeight, frame->u.get(), frame->uvWidth );
			}
			if( m_vChannel && m_vChannel->GetWidth() == frame->uvWidth && m_vChannel->GetHeight() == frame->uvHeight )
			{
				m_vChannel->UpdateSubresource( 0, 0, frame->uvWidth, frame->uvHeight, frame->v.get(), frame->uvWidth );
			}
			m_lastUpdatedTimeStamp = frame->timeStamp;
		}
	}
	else if( m_onCreateTextures )
	{
		auto frame = m_video->GetVideoFrame();
		if( frame )
		{
			m_onCreateTextures.CallVoid( this, std::make_pair( frame->yWidth, frame->yHeight ), std::make_pair( frame->uvWidth, frame->uvHeight ) );
		}
	}
	return errors;
}

VideoController::State VideoPlayer::GetState() const
{
	if( !m_video )
	{
		return VideoController::UNINITIALIZED;
	}
	return m_video->GetState();
}

void VideoPlayer::Pause()
{
	if( m_video )
	{
		m_video->Pause();
	}
}

void VideoPlayer::Resume()
{
	if( m_video )
	{
		m_video->Resume();
	}
}

bool VideoPlayer::IsPaused() const
{
	if( m_video )
	{
		return m_video->IsPaused();
	}
	return false;
}

VideoPlayerResult VideoPlayer::GetVideoInfo( std::map<std::string, uint32_t>& metadata )
{
	if( !m_video )
	{
		return VideoPlayerResult( "video not initialized" );
	}
	if( !m_video->GetParser().IsMetadataAvailable() )
	{
		return VideoPlayerResult( "video information is not yet parsed" );
	}
	auto& data = m_video->GetParser().GetVideoMetadata();
	metadata["width"] = data.width;
	metadata["height"] = data.height;
	return VideoPlayerResult();
}

VideoPlayerResult VideoPlayer::Validate()
{
	return m_lastError;
}

namespace
{

void ClearTexture( ITriTextureRes* texture, uint8_t fillColor )
{
	if( texture && texture->GetWidth() && texture->GetHeight() )
	{
		CcpMallocBuffer zeros( "VideoPlayer::ClearTextures", texture->GetWidth() * texture->GetHeight() );
		memset( zeros.get(), fillColor, zeros.size() );
		texture->UpdateSubresource( 0, 0, texture->GetWidth(), texture->GetHeight(), zeros.get(), texture->GetWidth() );
	}
}

}

void VideoPlayer::ClearTextures()
{
	ClearTexture( m_yChannel, 0 );
	ClearTexture( m_uChannel, 128 );
	ClearTexture( m_vChannel, 128 );
}

void VideoPlayer::OnTick( Be::Time realTime, Be::Time simTime, void* cookie )
{
	Update();
}