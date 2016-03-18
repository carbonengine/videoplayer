////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "stdafx.h"
#include "WebMParser.h"
#include "nestegg/nestegg.h"

namespace
{

const uint64_t AUDIO_QUEUE_LENGTH = 30000000000; // in nanoseconds
const size_t VIDEO_QUEUE_LENGTH = 30; // in number of frames


int Read( void * buffer, size_t length, void * userdata )
{
	auto read = static_cast<ICcpStream*>( userdata )->Read( buffer, length );
	if( read < 0 )
	{
		return -1;
	}
	return read == length ? 1 : 0;
}

int Seek(int64_t offset, int whence, void * userdata)
{
	return static_cast<ICcpStream*>( userdata )->Seek( ptrdiff_t( offset ), ICcpStream::SeekOrigin( whence ) ) >= 0 ? 0 : 1;
}

int64_t Tell(void * userdata)
{
	return static_cast<ICcpStream*>( userdata )->GetPosition();
}

void PopulateVideoMetadata( nestegg* nestEgg, unsigned trackIndex, VideoMetadata& metadata )
{
	if( trackIndex == -1 )
	{
		metadata = VideoMetadata();
		return;
	}

	auto codec = nestegg_track_codec_id( nestEgg, trackIndex );
	switch( codec )
	{
	case NESTEGG_CODEC_VP8:
		metadata.codec = VideoMetadata::VP8;
		break;
	case NESTEGG_CODEC_VP9:
		metadata.codec = VideoMetadata::VP9;
		break;
	default:
		metadata.codec = VideoMetadata::OTHER;
	}
	nestegg_video_params params;
	nestegg_track_video_params( nestEgg, trackIndex, &params );
	metadata.width = params.width;
	metadata.height = params.height;
	metadata.hasAlpha = params.alpha_mode != 0;
}

void PopulateAudioMetadata( nestegg* nestEgg, unsigned trackIndex, AudioMetadata& metadata )
{
	if( trackIndex == -1 )
	{
		metadata = AudioMetadata();
		return;
	}

	if( nestegg_track_codec_id( nestEgg, trackIndex ) == NESTEGG_CODEC_VORBIS )
	{
		metadata.codec = AudioMetadata::VORBIS;
	}
	nestegg_audio_params params;
	nestegg_track_audio_params( nestEgg, trackIndex, &params );
	metadata.channels = params.channels;
	metadata.bps = params.depth;
	metadata.rate = uint32_t( params.rate );

	unsigned nheaders = 0;
	auto r = nestegg_track_codec_data_count( nestEgg, trackIndex, &nheaders );
	if( r >= 0 && nheaders <= 3 ) 
	{
		for( unsigned header = 0; header < nheaders; ++header )
		{
			uint8_t* data;
			size_t length = 0;
			nestegg_track_codec_data( 
				nestEgg, 
				trackIndex, 
				header, 
				&data, 
				&metadata.codecInitializationData[header].length );
			metadata.codecInitializationData[header].data.reset( CCP_NEW( "PopulateAudioMetadata/data" ) uint8_t[metadata.codecInitializationData[header].length] );
			memcpy( metadata.codecInitializationData[header].data.get(), data, metadata.codecInitializationData[header].length );
		}
	}
}

}



NestEggFrame::NestEggFrame( nestegg_packet* packet, WebMParser* parser, uint64_t timeOffset )
	:m_packet( packet ),
	m_parser( parser ),
	m_timeOffset( timeOffset )
{
}

NestEggFrame::~NestEggFrame()
{
	m_parser->ReleasePacket( m_packet );
}

size_t NestEggFrame::GetFrameCount()
{
	unsigned count;
	nestegg_packet_count( m_packet, &count );
	return count;
}

uint64_t NestEggFrame::GetTimeStamp()
{
	uint64_t t;
	nestegg_packet_tstamp( m_packet, &t );
	return t + m_timeOffset;
}

bool NestEggFrame::GetFrame( size_t index, uint8_t*& data, size_t& length )
{
	return nestegg_packet_data( m_packet, unsigned( index ), &data, &length ) == 0;
}

bool NestEggFrame::GetAlphaFrame( uint8_t*& data, size_t& length )
{
	return nestegg_packet_additional_data( m_packet, 1, &data, &length ) == 0;
}



WebMParser::WebMParser( ICcpStream* videoStream, StreamType outputStreams, unsigned audioTrack, bool looped )
	:m_nestEgg( nullptr ),
	m_videoStream( videoStream ),
	m_metadataAvailable( false ),
	m_metadataMutex( "WebMParser", "m_metadataMutex" ),
	m_downloadedMediaTimeMutex( "WebMParser", "m_downloadedMediaTimeMutex", 100 ),
	m_videoTrack( -1 ),
	m_audioTrack( -1 ),
	m_requestedAudioTrack( audioTrack ),
	m_stopRequested( false ),
	m_looped( looped ),
	m_parserError( PARSER_ERROR_OK ),
	m_outputStreams( outputStreams ),
	m_duration( 0 ),
	m_downloadedMediaTime( 0 ),
	m_packets( "WebMParser::m_packets" )
{
	if( m_outputStreams & STREAM_VIDEO )
	{
		m_videoQueue.reset( CCP_NEW( "WebMParser.m_videoQueue" ) EncodedVideoFrameQueue( 
			MaxCountFullPolicy( m_outputStreams & STREAM_AUDIO ? std::numeric_limits<size_t>::max() : VIDEO_QUEUE_LENGTH ) ) );
	}
	if( m_outputStreams & STREAM_AUDIO )
	{
		m_audioQueue.reset( CCP_NEW( "WebMParser.m_audioQueue" ) EncodedAudioFrameQueue( MaxIntervalPolicy( AUDIO_QUEUE_LENGTH ) ) );
	}

	m_parseThread = CcpThread( &WebMParser::ReadThread, this );
	m_threadStarted.Wait();
}

WebMParser::~WebMParser()
{
	m_stopRequested = true;
	CompleteQueues();
	m_parseThread.join();
	m_audioQueue.reset();
	m_videoQueue.reset();
	for( auto it = m_packets.begin(); it != m_packets.end(); ++it )
	{
		nestegg_free_packet( *it );
	}
}

EncodedAudioFrameQueue* WebMParser::GetAudioQueue()
{
	return ( m_outputStreams & STREAM_AUDIO ) && m_audioTrack != -1 ? m_audioQueue.get() : nullptr;
}

EncodedVideoFrameQueue* WebMParser::GetVideoQueue()
{
	return ( m_outputStreams & STREAM_VIDEO ) && m_videoTrack != -1 ? m_videoQueue.get() : nullptr;
}

void WebMParser::CompleteQueues()
{
	if( m_audioQueue )
	{
		m_audioQueue->SetComplete();
	}
	if( m_videoQueue )
	{
		m_videoQueue->SetComplete();
	}
}

uint64_t WebMParser::GetDuration() const
{
	return m_duration;
}

uint64_t WebMParser::GetDownloadedMediaTime() const
{
	CcpAutoMutex lock( m_downloadedMediaTimeMutex );
	return m_downloadedMediaTime;
}

bool WebMParser::IsMetadataAvailable() const
{
	return m_metadataAvailable;
}

void WebMParser::WaitForMetadata() const
{
	m_metadataMutex.Acquire();
	m_metadataMutex.Release();
}

const VideoMetadata& WebMParser::GetVideoMetadata() const
{
	WaitForMetadata();
	return m_videoMetadata;
}

const AudioMetadata& WebMParser::GetAudioMetadata() const
{
	WaitForMetadata();
	return m_audioMetadata;
}

ParserError WebMParser::GetError() const
{
	return m_parserError;
}

void WebMParser::ReleasePacket( nestegg_packet* packet )
{
	if( !m_looped )
	{
		nestegg_free_packet( packet );
	}
}

unsigned WebMParser::FindTrack( int trackType, int trackIndex )
{
	unsigned trackCount = 0;
	nestegg_track_count( m_nestEgg, &trackCount );
	unsigned fallbackTrack = -1;
	unsigned foundTrack = 0;
	for( unsigned i = 0; i < trackCount; ++i )
	{
		if( nestegg_track_type( m_nestEgg, i ) == trackType )
		{
			if( foundTrack == 0 )
			{
				fallbackTrack = i;
			}
			if( foundTrack++ == trackIndex )
			{
				return i;
			}
		}
	}
	return fallbackTrack;
}

void WebMParser::ReadThread()
{
#ifndef WITH_TESTS
	CCP_STATS_ZONE( __FUNCTION__ );
#endif

	m_metadataMutex.Acquire();
	m_threadStarted.Signal();

	nestegg_io io;
	io.read = Read;
	io.seek = Seek;
	io.tell = Tell;
	io.userdata = m_videoStream;

	if( nestegg_init( &m_nestEgg, io, nullptr, -1 ) )
	{
		m_parserError = PARSER_ERROR_INVALID_STREAM;
		m_nestEgg = nullptr;
		m_metadataMutex.Release();
		return;
	}

	m_duration = 0;
	nestegg_duration( m_nestEgg, &m_duration );

	m_videoTrack = FindTrack( NESTEGG_TRACK_VIDEO );
	PopulateVideoMetadata( m_nestEgg, m_videoTrack, m_videoMetadata );
	m_audioTrack = FindTrack( NESTEGG_TRACK_AUDIO, m_requestedAudioTrack );
	PopulateAudioMetadata( m_nestEgg, m_audioTrack, m_audioMetadata );

	if( m_audioTrack == -1 && m_videoQueue )
	{
		m_videoQueue->GetFullPolicy().SetMaxCount( VIDEO_QUEUE_LENGTH );
	}

	m_metadataMutex.Release();
	m_metadataAvailable = true;

	uint64_t timeOffset = 0;

	nestegg_packet* packet = 0;
	while( !m_stopRequested )
	{
		auto r = nestegg_read_packet( m_nestEgg, &packet );
		if( r == 1 && packet == 0 )
		{
			continue;
		}
		if( r < 0 )
		{
			m_parserError = PARSER_ERROR_INVALID_DATA;
			break;
		}
		else if( r == 0 )
		{
			break;
		}

		unsigned int track = 0;
		nestegg_packet_track( packet, &track );
		uint64_t packetTime = 0;

		if( nestegg_track_type( m_nestEgg, track ) == NESTEGG_TRACK_VIDEO ) 
		{
			if( track == m_videoTrack && m_videoQueue )
			{
				m_videoQueue->Push( CCP_NEW( "WebMParser/video frame" ) NestEggFrame( packet, this, timeOffset ) );
				if( m_looped )
				{
					m_packets.push_back( packet );
				}
				if( nestegg_packet_tstamp( packet, &packetTime ) == 0 )
				{
					CcpAutoMutex lock( m_downloadedMediaTimeMutex );
					m_downloadedMediaTime = packetTime;
				}
			}
			else
			{
				nestegg_free_packet( packet );
			}
		}
		else
		{
			if( track == m_audioTrack && m_audioQueue )
			{
				m_audioQueue->Push( CCP_NEW( "WebMParser/audio frame" ) NestEggFrame( packet, this, timeOffset ) );
				if( m_looped )
				{
					m_packets.push_back( packet );
				}
				if( nestegg_packet_tstamp( packet, &packetTime ) == 0 )
				{
					CcpAutoMutex lock( m_downloadedMediaTimeMutex );
					m_downloadedMediaTime = packetTime;
				}
			}
			else
			{
				nestegg_free_packet( packet );
			}
		}
	}
	timeOffset += m_duration;
	while( m_looped && !m_stopRequested )
	{
		for( auto it = m_packets.begin(); it != m_packets.end() && !m_stopRequested; ++it )
		{
			auto packet = *it;
			unsigned int track = 0;
			nestegg_packet_track( packet, &track );

			if( nestegg_track_type( m_nestEgg, track ) == NESTEGG_TRACK_VIDEO ) 
			{
				if( track == m_videoTrack && m_videoQueue )
				{
					m_videoQueue->Push( CCP_NEW( "WebMParser/video frame" ) NestEggFrame( packet, this, timeOffset ) );
				}
			}
			else
			{
				if( track == m_audioTrack && m_audioQueue )
				{
					m_audioQueue->Push( CCP_NEW( "WebMParser/audio frame" ) NestEggFrame( packet, this, timeOffset ) );
				}
			}
		}
		timeOffset += m_duration;
	}


	if( m_videoQueue )
	{
		m_videoQueue->SetComplete();
	}
	if( m_audioQueue )
	{
		m_audioQueue->SetComplete();
	}
	nestegg_destroy( m_nestEgg );
	m_nestEgg = nullptr;
}