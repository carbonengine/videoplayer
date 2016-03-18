#include "StdAfx.h"
#include "TestUtilities.h"

TestMemoryStream::TestMemoryStream( const void* begin, const void* end )
	:m_begin( static_cast<const uint8_t*>( begin ) ),
	m_end( static_cast<const uint8_t*>( end ) ),
	m_position( static_cast<const uint8_t*>( begin ) ),
	m_stall( false )
{
}

void TestMemoryStream::StallOperations()
{
	m_stall = true;
}

void TestMemoryStream::ResumeOperations()
{
	m_stall = false;
}

ptrdiff_t TestMemoryStream::Read( void* dest, ptrdiff_t count )
{
	StallIfRequested();
	count = std::min( count, m_end - m_position );
	memcpy( dest, m_position, count );
	m_position += count;
	return count;
}
	
ptrdiff_t TestMemoryStream::Write( const void* source, size_t count )
{
	StallIfRequested();
	return -1;
}

ptrdiff_t TestMemoryStream::Seek( ptrdiff_t distance, SeekOrigin method )
{
	StallIfRequested();

	const uint8_t* position;

	switch( method )
	{
	case SO_BEGIN:
		position = m_begin + distance;
		break;
	case SO_CURRENT:
		position = m_position + distance;
		break;
	case SO_END:
		position = m_end - distance;
		break;
	default:
		return -1;
	}
	if( position < m_begin || position >= m_end )
	{
		return -1;
	}
	m_position = position;
	return m_position - m_begin;
}

ptrdiff_t TestMemoryStream::GetPosition()
{
	StallIfRequested();

	return m_position - m_begin;
}

ptrdiff_t TestMemoryStream::GetSize()
{
	StallIfRequested();

	return m_end - m_begin;
}

void TestMemoryStream::StallIfRequested()
{
	while( m_stall )
	{
		CcpThreadSleep( 0 );
	}
}


TestEncodedFrame::TestEncodedFrame( const uint8_t* frameData, size_t size )
	:m_data( frameData ),
	m_size( size )
{
}

size_t TestEncodedFrame::GetFrameCount()
{
	return 1;
}

uint64_t TestEncodedFrame::GetTimeStamp()
{
	return 0;
}

bool TestEncodedFrame::GetFrame( size_t index, uint8_t*& data, size_t& length )
{
	length = m_size;
	data = (uint8_t*)( m_data );
	return true;
}

bool TestEncodedFrame::GetAlphaFrame( uint8_t*& data, size_t& length )
{
	return false;
}
