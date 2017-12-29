////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "stdafx.h"
#include "VorbisDecoder.h"
#include "nestegg/nestegg.h"
#include "WebMParser.h"


namespace
{

const size_t QUEUE_LENGTH = 30;

void ResampleTo16Bps( float** pcm, uint32_t channels, uint32_t samples, int16_t* data )
{
	for( uint32_t j = 0; j < channels; ++j )
	{
		for( uint32_t i = 0; i < samples; ++i )
		{
			int val = int( pcm[j][i] * 32767.0f + 0.5f );
			if( val > 32767 )
			{
				val = 32767;
            }
            if( val < -32768 )
			{
				val = -32768;
            } 

			data[i * channels + j] = int16_t( val );
		}
	}
}

}

VorbisDecoder::VorbisDecoder( const AudioMetadata& audioMetadata, EncodedAudioFrameQueue& compressedQueue )
	:m_audioMetadata( audioMetadata ),
	m_compressedQueue( compressedQueue ),
	m_decodedQueue( MaxCountFullPolicy( QUEUE_LENGTH ), FrameDeleter<PcmFrame>( this ) ),
	m_poolMutex( "VorbisDecoder", "m_poolMutex" ),
	m_stopRequested( false ),
	m_initializeState( NONE ),
	m_error( DECODER_ERROR_UNSUPPORTED_CODEC ),
	m_processedFrames( 0 ),
	m_corruptFrames( 0 )
{
	if( m_audioMetadata.codec != AudioMetadata::VORBIS )
	{
		return;
	}

	vorbis_info_init( &m_vorbisInfo );
	vorbis_comment_init( &m_vorbisComment );
	m_initializeState = INFO;

	for( unsigned header = 0; header < 3; ++header )
	{
		ogg_packet packet;
		packet.packet = m_audioMetadata.codecInitializationData[header].data.get();
		packet.bytes = long( m_audioMetadata.codecInitializationData[header].length );
		packet.b_o_s = header == 0;
		packet.e_o_s = false;
		packet.granulepos = 0;
		packet.packetno = header;

		auto r = vorbis_synthesis_headerin( &m_vorbisInfo, &m_vorbisComment, &packet );
		if( r != 0 )
		{
			return;
		}
	}

	auto r = vorbis_synthesis_init( &m_vorbisDsp, &m_vorbisInfo );
	if( r != 0 )
	{
		return;
	}

	r = vorbis_block_init( &m_vorbisDsp, &m_vorbisBlock );
	if( r != 0 )
	{
		return;
	}
	m_initializeState = BLOCK;
	m_error = DECODER_ERROR_OK;

	m_decodeThread = CcpThread( &VorbisDecoder::DecodeThread, this );
}

VorbisDecoder::~VorbisDecoder()
{
	m_decodedQueue.SetComplete();
	m_stopRequested = true;
	WaitUntilDone();
	switch( m_initializeState )
	{
	case BLOCK:
		vorbis_block_clear( &m_vorbisBlock );
	case INFO:
		vorbis_comment_clear( &m_vorbisComment );
		vorbis_info_clear( &m_vorbisInfo );
	default:
		break;
	}
}

PcmFrameQueue& VorbisDecoder::GetDecodedQueue()
{
	return m_decodedQueue;
}

DecoderError VorbisDecoder::GetError() const
{
	return m_error;
}

uint32_t VorbisDecoder::GetProcessedFrameCount() const
{
	return m_processedFrames;
}

uint32_t VorbisDecoder::GetCorruptFrameCount() const
{
	return m_corruptFrames;
}

void VorbisDecoder::WaitUntilDone()
{
    if( m_decodeThread.joinable() )
    {
        m_decodeThread.join();
    }
}

void VorbisDecoder::DecodeThread()
{
#ifndef WITH_TESTS
	CCP_STATS_ZONE( __FUNCTION__ );
#endif

	int packetCount = 3;
	bool firstFrame = true;

	while( !m_stopRequested )
	{
		auto packet = m_compressedQueue.Pop();
		if( !packet )
		{
			m_decodedQueue.SetComplete();
			break;
		}

		if( packet->IsSeekFrame() )
		{
			m_decodedQueue.Clear();
			firstFrame = true;
			continue;
		}

		if( packet->IsSkipFrame() )
		{
			continue;
		}

		auto count = packet->GetFrameCount();
		int nframes = 0;

		for( unsigned j = 0; !m_stopRequested && j < count; ++j ) 
		{
			uint8_t* data;
			size_t length;
			if( !packet->GetFrame( j, data, length ) )
			{
				continue;
			}

			ogg_packet oggPacket;
			oggPacket.packet = const_cast<unsigned char*>(data);
			oggPacket.bytes = long( length );
			oggPacket.b_o_s = false;
			oggPacket.e_o_s = false;
			oggPacket.granulepos = -1;
			oggPacket.packetno = packetCount++;

			if( vorbis_synthesis( &m_vorbisBlock, &oggPacket ) != 0 ) 
			{
				++m_corruptFrames;
				continue;
			}

			if( vorbis_synthesis_blockin( &m_vorbisDsp, &m_vorbisBlock ) != 0 ) 
			{
				continue;
			}
			float **pcm; 
			int32_t samples = vorbis_synthesis_pcmout( &m_vorbisDsp, &pcm );
			if( firstFrame )
			{
				PcmFrame* pcmFrame = GetNewFrame( 0, 0 );
				pcmFrame->timeStamp = packet->GetTimeStamp();
				pcmFrame->samples = 0;
				pcmFrame->channels = 0;
				m_decodedQueue.Push( pcmFrame );
			}
			++m_processedFrames;
			firstFrame = false;
			while( !m_stopRequested && samples > 0 )
			{
				PcmFrame* pcmFrame = GetNewFrame( m_audioMetadata.channels, samples );
				pcmFrame->timeStamp = packet->GetTimeStamp();
				pcmFrame->samples = samples;
				pcmFrame->channels = m_audioMetadata.channels;
				ResampleTo16Bps( pcm, m_audioMetadata.channels, samples, pcmFrame->data );
				m_decodedQueue.Push( pcmFrame );

				if( vorbis_synthesis_read( &m_vorbisDsp, samples ) != 0 ) 
				{
					break;
				}
				samples = vorbis_synthesis_pcmout( &m_vorbisDsp, &pcm );
			}
		}
	}
}

PcmFrame* VorbisDecoder::GetNewFrame( uint32_t channels, uint32_t samples )
{
	CcpAutoMutex lock( m_poolMutex );
	if( !m_framePool.empty() )
	{
		for( size_t i = m_framePool.size(); i > 0; --i )
		{
			auto& ptr = m_framePool[i - 1];
			if( ptr->channels == channels && ptr->samples == samples )
			{
				auto frame = ptr.release();
				m_framePool.erase( m_framePool.begin() + i - 1 );
				return frame;
			}
		}
	}
	return new( channels, samples )  PcmFrame;
}

void VorbisDecoder::ReleaseFrame( PcmFrame* frame )
{
	CcpAutoMutex lock( m_poolMutex );
	m_framePool.push_back( std::unique_ptr<PcmFrame>( frame ) );
}
