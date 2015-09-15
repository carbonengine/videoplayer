////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "stdafx.h"
#include "VpxDecoder.h"
#include "vp8dx.h"

namespace
{

const size_t QUEUE_LENGTH = 5;


inline int ScaleYuv( int v )
{
	return ( v + 128000 ) / 256000;
}

inline int RCoeff( int y, int u, int v )
{
	return 298082 * y + 0 * u + 408583*v; 
}

inline int GCoeff( int y, int u, int v )
{ 
	return 298082 * y - 100291 * u - 208120 * v; 
}

inline int BCoeff( int y, int u, int v )
{
	return 298082 * y + 516411 * u + 0 * v; 
}

uint8_t Clamp( int vv )
{
	if( vv < 0 )
	{
		return 0;
	}
	else if( vv > 255 )
	{
		return 255;
	}
	return uint8_t( vv );
}

bool IsKeyFrame( vpx_codec_ctx_t* context )
{
	vpx_codec_stream_info_t info;
	memset( &info, 0, sizeof( info ) );
	info.sz = sizeof( info );
	vpx_codec_get_stream_info( context, &info );
	return info.is_kf != 0;
}

inline void CopyImagePlane( uint8_t* dest, uint8_t* source, int sourceStride, uint32_t width, uint32_t height )
{
	for( uint32_t i = 0; i < height; ++i )
	{
		memcpy( dest + width * i, source + sourceStride * i, width );
	}
}

}

VpxDecoder::VpxDecoder( const VideoMetadata& videoMetadata, EncodedVideoFrameQueue& compressedQueue )
	:m_videoMetadata( videoMetadata ),
	m_compressedQueue( compressedQueue ),
	m_decompressedQueue( QUEUE_LENGTH ),
	m_dropFrameTime( 0 ),
	m_stopRequested( false ),
	m_error( DECODER_ERROR_OK ),
	m_corruptFrames( 0 ),
	m_droppedFrames( 0 ),
	m_processedFrames( 0 )
{
	if( m_videoMetadata.codec != VideoMetadata::VP8 && m_videoMetadata.codec != VideoMetadata::VP9 )
	{
		m_error = DECODER_ERROR_UNSUPPORTED_CODEC;
		return;
	}
	m_decodeThread = CcpThread( &VpxDecoder::DecodeThread, this );
}

VpxDecoder::~VpxDecoder()
{
	m_decompressedQueue.SetComplete();
	m_stopRequested = true;
	WaitUntilDone();
}

VideoFrameQueue& VpxDecoder::GetDecodedQueue()
{
	return m_decompressedQueue;
}

void VpxDecoder::SetDropFrameTime( uint64_t time )
{
	m_dropFrameTime = time;
}

DecoderError VpxDecoder::GetError() const
{
	return m_error;
}

uint32_t VpxDecoder::GetProcessedFrameCount() const
{
	return m_processedFrames;
}

uint32_t VpxDecoder::GetCorruptFrameCount() const
{
	return m_corruptFrames;
}

uint32_t VpxDecoder::GetDroppedFrameCount() const
{
	return m_droppedFrames;
}

void VpxDecoder::WaitUntilDone()
{
    if( m_decodeThread.joinable() )
    {
        m_decodeThread.join();
    }
}

void VpxDecoder::DecodeThread()
{
#ifndef WITH_TESTS
	CCP_STATS_ZONE( __FUNCTION__ );
#endif

	auto videoInterface = m_videoMetadata.codec == VideoMetadata::VP8 ? &vpx_codec_vp8_dx_algo : &vpx_codec_vp9_dx_algo;
	vpx_codec_dec_init( &m_videoCodec, videoInterface, nullptr, 0 );

	while( !m_stopRequested )
	{
		auto packet = m_compressedQueue.Pop();
		if( !packet )
		{
			m_decompressedQueue.SetComplete();
			break;
		}

		auto count = packet->GetFrameCount();
		uint64_t packetTimeStamp = packet->GetTimeStamp();

		for( unsigned j = 0; !m_stopRequested && j < count; ++j ) 
		{
			uint8_t* data;
			size_t length;
			if( !packet->GetFrame( j, data, length ) )
			{
				continue;
			}

			vpx_codec_err_t e = vpx_codec_decode( &m_videoCodec, data, unsigned( length ), nullptr, 0 );
			if( e ) 
			{
				++m_corruptFrames;
				continue;
			}

			if( packetTimeStamp < m_dropFrameTime && !IsKeyFrame( &m_videoCodec ) )
			{
				++m_droppedFrames;
				continue;
			}

			vpx_codec_iter_t  iter = nullptr;
			while( auto img = vpx_codec_get_frame( &m_videoCodec, &iter ) )
			{
				auto frame = CCP_NEW( "VpxDecoder/frame" ) VideoFrame;
				frame->width = img->d_w;
				frame->height = img->d_h;
				frame->timeStamp = packetTimeStamp;
				frame->y.reset( CCP_NEW( "VpxDecoder/frame/y" ) uint8_t[frame->width * frame->height] );
				CopyImagePlane( frame->y.get(), img->planes[0], img->stride[0], frame->width, frame->height );
				frame->u.reset( CCP_NEW( "VpxDecoder/frame/u" ) uint8_t[frame->width * frame->height / 4] );
				CopyImagePlane( frame->u.get(), img->planes[1], img->stride[1], frame->width / 2, frame->height / 2 );
				frame->v.reset( CCP_NEW( "VpxDecoder/frame/v" ) uint8_t[frame->width * frame->height / 4] );
				CopyImagePlane( frame->v.get(), img->planes[2], img->stride[2], frame->width / 2, frame->height / 2 );
				m_decompressedQueue.Push( frame );
				++m_processedFrames;
			}
		}
	}
	vpx_codec_destroy( &m_videoCodec );
}


void YuvFrameToBgrx( const VideoFrame& frame, uint8_t* pixels ) 
{
	const int w = frame.width;
	const int h = frame.height;

	auto py = frame.y.get();
	auto pu = frame.u.get();
	auto pv = frame.v.get();
	auto rgb = pixels;
	for( int j = 0; j < h; ++j )
	{
		for( int i = 0; i < w; ++i ) 
		{
			int y = py[i] - 16;
			int u = pu[i/2] - 128;
			int v = pv[i/2] - 128;
			rgb[2] = Clamp( ScaleYuv( RCoeff( y, u, v ) ) );
			rgb[1] = Clamp( ScaleYuv( GCoeff( y, u, v ) ) );
			rgb[0] = Clamp( ScaleYuv( BCoeff( y, u, v ) ) );
			rgb += 4;
		}
		py += frame.width;
		if( j & 1 )
		{
			pu += frame.width / 2;
			pv += frame.width / 2;
		}
	}
}
