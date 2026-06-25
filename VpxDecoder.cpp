////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "VpxDecoder.h"
#include <vpx/vp8dx.h>

namespace
{

const size_t QUEUE_LENGTH = 5;
const int K1 = int( 1.164 * ( 1 << 16 ) );
const int K2 = int( 2.018 * ( 1 << 16 ) );
const int K3 = int( 0.813 * ( 1 << 16 ) );
const int K4 = int( 0.391 * ( 1 << 16 ) );
const int K5 = int( 1.596 * ( 1 << 16 ) );


inline int ScaleYuv( int v )
{
	return ( v + ( 1 << 15 ) ) >> 16;
}

inline int RCoeff( int y, int u, int v )
{
	return K1 * y + 0 * u + K2 * v; 
}

inline int GCoeff( int y, int u, int v )
{ 
	return K1 * y - K3 * u - K4 * v; 
}

inline int BCoeff( int y, int u, int v )
{
	return K1 * y + K5 * u + 0 * v; 
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


void YuvToBgra444( 
	uint8_t* dest, 
	const uint8_t* srcY,
	int yStride,
	const uint8_t* srcU, 
	int uStride,
	const uint8_t* srcV, 
	int vStride,
	const uint8_t* srcAlpha, 
	int alphaStride,
	uint32_t width, 
	uint32_t height )
{
	yStride -= int( width );
	uStride -= int( width );
	vStride -= int( width );
	alphaStride -= int( width );
	for( uint32_t j = 0; j < height; ++j )
	{
		for( uint32_t i = 0; i < width; ++i )
		{
			int y = int( *srcY++ ) - 16;
			int u = int( *srcU++ ) - 128;
			int v = int( *srcV++ ) - 128;
			*dest++ = Clamp( ScaleYuv( BCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( GCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( RCoeff( y, u, v ) ) );
			*dest++ = *srcAlpha++;
		}
		srcY += yStride;
		srcU += uStride;
		srcV += vStride;
		srcAlpha += alphaStride;
	}
}

void YuvToBgrx444( 
	uint8_t* dest, 
	const uint8_t* srcY,
	int yStride,
	const uint8_t* srcU, 
	int uStride,
	const uint8_t* srcV, 
	int vStride,
	uint32_t width, 
	uint32_t height )
{
	yStride -= int( width );
	uStride -= int( width );
	vStride -= int( width );
	for( uint32_t j = 0; j < height; ++j )
	{
		for( uint32_t i = 0; i < width; ++i )
		{
			int y = int( *srcY++ ) - 16;
			int u = int( *srcU++ ) - 128;
			int v = int( *srcV++ ) - 128;
			*dest++ = Clamp( ScaleYuv( BCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( GCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( RCoeff( y, u, v ) ) );
			*dest++ = 0xff;
		}
		srcY += yStride;
		srcU += uStride;
		srcV += vStride;
	}
}

void YuvToBgra422( 
	uint8_t* dest, 
	const uint8_t* srcY,
	int yStride,
	const uint8_t* srcU, 
	int uStride,
	const uint8_t* srcV, 
	int vStride,
	const uint8_t* srcAlpha, 
	int alphaStride,
	uint32_t width, 
	uint32_t height )
{
	yStride -= int( width );
	uStride -= int( ( width + 1 ) / 2 );
	vStride -= int( ( width + 1 ) / 2 );
	alphaStride -= int( width );
	for( uint32_t j = 0; j < height; ++j )
	{
		auto uRow = srcU;
		auto vRow = srcV;
		int u, v;
		for( uint32_t i = 0; i < width; ++i )
		{
			int y = int( *srcY++ ) - 16;
			if( ( i & 1 ) == 0 )
			{
				u = int( *uRow++ ) - 128;
				v = int( *vRow++ ) - 128;
			}
			*dest++ = Clamp( ScaleYuv( BCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( GCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( RCoeff( y, u, v ) ) );
			*dest++ = *srcAlpha++;
		}
		srcY += yStride;
		srcAlpha += alphaStride;
		if( ( j & 1 ) != 0 && j / 2 + 1 < height / 2 )
		{
			srcU = uRow;
			srcV = vRow;
			srcU += uStride;
			srcV += vStride;
		}
	}
}

void YuvToBgrx422( 
	uint8_t* dest, 
	const uint8_t* srcY,
	int yStride,
	const uint8_t* srcU, 
	int uStride,
	const uint8_t* srcV, 
	int vStride,
	uint32_t width, 
	uint32_t height )
{
	yStride -= int( width );
	uStride -= int( ( width + 1 ) / 2 );
	vStride -= int( ( width + 1 ) / 2 );
	for( uint32_t j = 0; j < height; ++j )
	{
		auto uRow = srcU;
		auto vRow = srcV;
		int u, v;
		for( uint32_t i = 0; i < width; ++i )
		{
			int y = int( *srcY++ ) - 16;
			if( ( i & 1 ) == 0 )
			{
				u = int( *uRow++ ) - 128;
				v = int( *vRow++ ) - 128;
			}
			*dest++ = Clamp( ScaleYuv( BCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( GCoeff( y, u, v ) ) );
			*dest++ = Clamp( ScaleYuv( RCoeff( y, u, v ) ) );
			*dest++ = 0xff;
		}
		srcY += yStride;
		if( ( j & 1 ) != 0 && j / 2 + 1 < height / 2 )
		{
			srcU = uRow;
			srcV = vRow;
			srcU += uStride;
			srcV += vStride;
		}
	}
}

void YuvToBgra(
	uint8_t* dest,
	VideoFrame::Color& averageColor,
	const uint8_t* srcY, 
	int yStride,
	const uint8_t* srcU, 
	int uStride,
	const uint8_t* srcV, 
	int vStride,
	const uint8_t* srcAlpha, 
	int alphaStride,
	uint32_t width, 
	uint32_t height, 
	uint32_t uvShift )
{
	if( uvShift )
	{
		if( srcAlpha )
		{
			YuvToBgra422( dest, srcY, yStride, srcU, uStride, srcV, vStride, srcAlpha, alphaStride, width, height );
		}
		else
		{
			YuvToBgrx422( dest, srcY, yStride, srcU, uStride, srcV, vStride, width, height );
		}
	}
	else
	{
		if( srcAlpha )
		{
			YuvToBgra444( dest, srcY, yStride, srcU, uStride, srcV, vStride, srcAlpha, alphaStride, width, height );
		}
		else
		{
			YuvToBgrx444( dest, srcY, yStride, srcU, uStride, srcV, vStride, width, height );
		}
	}

	uint32_t numPixels = width * height;
	uint32_t r = 0;
	uint32_t g = 0;
	uint32_t b = 0;
	uint32_t a = 0;
	for( uint32_t i = 0; i < numPixels; ++i )
	{
		b += *dest++;
		g += *dest++;
		r += *dest++;
		a += *dest++;
	}
	averageColor.red = float( double( r ) / 255. / numPixels );
	averageColor.green = float( double( g ) / 255. / numPixels );
	averageColor.blue = float( double( b ) / 255. / numPixels );
	averageColor.alpha = float( double( a ) / 255. / numPixels );
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
	m_decompressedQueue( QUEUE_LENGTH, FrameDeleter<VideoFrame>( this ) ),
	m_yuvFrameQueue( QUEUE_LENGTH, FrameDeleter<YuvFrame>( this ) ),
	m_poolMutex( "VpxDecoder", "m_poolMutex" ),
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
	m_convertThread = CcpThread( &VpxDecoder::ConvertThread, this );
	m_decodeThread = CcpThread( &VpxDecoder::DecodeThread, this );
}

VpxDecoder::~VpxDecoder()
{
	m_decompressedQueue.SetComplete();
	m_yuvFrameQueue.SetComplete();
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
    if( m_convertThread.joinable() )
    {
        m_convertThread.join();
    }
}

void VpxDecoder::DecodeThread()
{
#ifndef WITH_TESTS
	CCP_STATS_ZONE( __FUNCTION__ );
#endif

	auto videoInterface = m_videoMetadata.codec == VideoMetadata::VP8 ? &vpx_codec_vp8_dx_algo : &vpx_codec_vp9_dx_algo;
	vpx_codec_dec_init( &m_videoCodec, videoInterface, nullptr, 0 );

	bool alphaInitialized = false;
	bool hasAlpha = false;

	while( !m_stopRequested )
	{
		auto packet = m_compressedQueue.Pop();
		if( !packet )
		{
			m_yuvFrameQueue.SetComplete();
			break;
		}

		if( packet->IsSeekFrame() )
		{
			m_decompressedQueue.Clear();
			m_yuvFrameQueue.Clear();
			m_yuvFrameQueue.Push( new( 0, 0, 0, false )  YuvFrame( 0, 0, 0, false ) );
			m_dropFrameTime = 0;
			vpx_codec_destroy( &m_videoCodec );
			vpx_codec_dec_init( &m_videoCodec, videoInterface, nullptr, 0 );
			if( hasAlpha )
			{
				vpx_codec_destroy( &m_alphaCodec );
				vpx_codec_dec_init( &m_alphaCodec, &vpx_codec_vp8_dx_algo, nullptr, 0 );
			}
			continue;
		}

		auto count = packet->GetFrameCount();
		uint64_t packetTimeStamp = packet->GetTimeStamp();

		for( unsigned j = 0; !m_stopRequested && j < count; ++j ) 
		{
			uint8_t* alphaData;
			size_t alphaLength;

			if( packet->GetAlphaFrame( alphaData, alphaLength ) )
			{
				if( !alphaInitialized )
				{
					vpx_codec_dec_init( &m_alphaCodec, &vpx_codec_vp8_dx_algo, nullptr, 0 );
					hasAlpha = true;
					alphaInitialized = true;
				}
				if( hasAlpha )
				{
					vpx_codec_err_t e = vpx_codec_decode( &m_alphaCodec, alphaData, unsigned( alphaLength ), nullptr, 0 );
					if( e ) 
					{
						++m_corruptFrames;
						continue;
					}
				}
			}
			else
			{
				alphaInitialized = true;
			}

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

			if( packet->IsSkipFrame() )
			{
				m_dropFrameTime = 0;
				continue;
			}

			if( ( packetTimeStamp < m_dropFrameTime ) && ( !IsKeyFrame( &m_videoCodec )  || m_dropFrameTime - packetTimeStamp > 10000000 ) )
			{
				++m_droppedFrames;
				continue;
			}

			vpx_codec_iter_t  iter = nullptr;
			while( auto img = vpx_codec_get_frame( &m_videoCodec, &iter ) )
			{
				auto frame = GetNewYuvFrame( img->d_w, img->d_h, img->x_chroma_shift, hasAlpha );
				frame->timeStamp = packetTimeStamp;
				vpx_image_t* alphaImg = nullptr;
				if( hasAlpha )
				{
					vpx_codec_iter_t  alphaIter = nullptr;
					alphaImg = vpx_codec_get_frame( &m_alphaCodec, &alphaIter );
				}

				auto uvWidth = frame->width >> img->x_chroma_shift;
				auto uvHeight = frame->height >> img->x_chroma_shift;

				CopyImagePlane( frame->y, img->planes[0], img->stride[0], frame->width, frame->height );
				CopyImagePlane( frame->u, img->planes[1], img->stride[1], uvWidth, uvHeight );
				CopyImagePlane( frame->v, img->planes[2], img->stride[2], uvWidth, uvHeight );

				if( alphaImg )
				{
					CopyImagePlane( frame->alpha, alphaImg->planes[0], alphaImg->stride[0], frame->width, frame->height );
				}

				m_yuvFrameQueue.Push( frame );
				++m_processedFrames;
			}
		}
	}
	vpx_codec_destroy( &m_videoCodec );
	if( hasAlpha )
	{
		vpx_codec_destroy( &m_alphaCodec );
	}
}

void VpxDecoder::ConvertThread()
{
#ifndef WITH_TESTS
	CCP_STATS_ZONE( __FUNCTION__ );
#endif
	while( !m_stopRequested )
	{
		auto yuv = m_yuvFrameQueue.Pop();
		if( !yuv )
		{
			m_decompressedQueue.SetComplete();
			break;
		}

		if( yuv->width == 0 )
		{
			m_decompressedQueue.Clear();
			continue;
		}

		if( ( yuv->timeStamp < m_dropFrameTime ) && m_yuvFrameQueue.Size() )
		{
			++m_droppedFrames;
			continue;
		}

		auto frame = GetNewVideoFrame( yuv->width, yuv->height );
		if( !frame )
		{
			++m_corruptFrames;
			continue;
		}
		frame->width = yuv->width;
		frame->height = yuv->height;
		frame->timeStamp = yuv->timeStamp;

		YuvToBgra( 
			frame->bgra, 
			frame->averageColor,
			yuv->y,
			yuv->width,
			yuv->u,
			yuv->width >> yuv->uvShift,
			yuv->v,
			yuv->width >> yuv->uvShift,
			yuv->alpha,
			yuv->width,
			frame->width, frame->height, yuv->uvShift );
					
		m_decompressedQueue.Push( frame );
	}
}

YuvFrame::YuvFrame( uint32_t width_, uint32_t height_, uint32_t uvShift_, bool alpha_ )
	:width( width_ ),
	height( height_ ),
	uvShift( uvShift_ )
{
	size_t sz = sizeof( YuvFrame );
	size_t ySize = width * height;
	size_t uvSize = ySize >> ( uvShift * 2 );
	uint8_t* offset = reinterpret_cast<uint8_t*>( this );
	offset += sz;
	y = offset;
	offset += ySize;
	u = offset;
	offset += uvSize;
	v = offset;
	if( alpha_ )
	{
		offset += uvSize;
		alpha = offset;
	}
	else
	{
		alpha = nullptr;
	}
}

void* YuvFrame::operator new( std::size_t sz, uint32_t width, uint32_t height, uint32_t uvShift, bool alpha )
{
	size_t ySize = width * height;
	size_t uvSize = ySize >> ( uvShift * 2 );
	size_t size = sz + ySize + uvSize * 2;
	if( alpha )
	{
		size += ySize;
	}
	return CCP_MALLOC( "VpxDecoder::YuvFrame", size );
}

void YuvFrame::operator delete( void* ptr )
{
	CCP_FREE( ptr );
}


YuvFrame* VpxDecoder::GetNewYuvFrame( uint32_t width, uint32_t height, uint32_t uvShift, bool alpha )
{
	CcpAutoMutex lock( m_poolMutex );
	if( m_yuvFramePool.empty() )
	{
		return new( width, height, uvShift, alpha )  YuvFrame( width, height, uvShift, alpha );
	}
	else
	{
		auto frame = m_yuvFramePool.back().release();
		m_yuvFramePool.pop_back();
		return frame;
	}
}

VideoFrame* VpxDecoder::GetNewVideoFrame( uint32_t width, uint32_t height )
{
	CcpAutoMutex lock( m_poolMutex );
	if( m_videoFramePool.empty() )
	{
		return new( width, height )  VideoFrame;
	}
	else
	{
		auto frame = m_videoFramePool.back().release();
		m_videoFramePool.pop_back();
		return frame;
	}
}

void VpxDecoder::ReleaseFrame( YuvFrame* frame )
{
	if( !frame->width )
	{
		CCP_DELETE frame;
	}
	else
	{
		CcpAutoMutex lock( m_poolMutex );
		m_yuvFramePool.push_back( std::unique_ptr<YuvFrame>( frame ) );
	}
}

void VpxDecoder::ReleaseFrame( VideoFrame* frame )
{
	CcpAutoMutex lock( m_poolMutex );
	m_videoFramePool.push_back( std::unique_ptr<VideoFrame>( frame ) );
}
