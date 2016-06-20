#pragma once

#include "Metadata.h"


class TestMemoryStream: public ICcpStream
{
public:
	TestMemoryStream( const void* begin, const void* end );

	void StallOperations();
	void ResumeOperations();

	virtual ptrdiff_t Read( void* dest, ptrdiff_t count );
	virtual ptrdiff_t Write( const void* source, size_t count );
	virtual ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method );
	virtual ptrdiff_t GetPosition();
	virtual ptrdiff_t GetSize();
private:
	void StallIfRequested();

	const uint8_t* m_begin;
	const uint8_t* m_end;
	const uint8_t* m_position;
	volatile bool m_stall;
};


class TestEncodedFrame: public IEncodedFrame
{
public:
	TestEncodedFrame( const uint8_t* frameData, size_t size );
	virtual size_t GetFrameCount();
	virtual uint64_t GetTimeStamp();
	virtual bool IsSeekFrame() const;
	virtual bool GetFrame( size_t index, uint8_t*& data, size_t& length );
	virtual bool GetAlphaFrame( uint8_t*& data, size_t& length );
private:
	const uint8_t* m_data;
	size_t m_size;
};

template <typename Queue>
void WaitForQueueEmpty( Queue& queue, uint32_t timeoutMS = 3000 )
{
	for( uint32_t i = 0; i < timeoutMS; ++i )
	{
		if( queue.Size() == 0 )
		{
			break;
		}
		CcpThreadSleep( 1 );
	}
}

template <typename Operation>
void Wait( Operation operation, uint32_t timeoutMS )
{
	CcpSemaphore s( 0, 1 );
	CcpThread t( [&] 
	{ 
		operation(); 
		s.Signal(); 
	} );
	if( !s.TimedWait( timeoutMS ) )
	{
		CcpKillThread( t.native_handle() );
		t.detach();
	}
	else
	{
		t.join();
	}
}