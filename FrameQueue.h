////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef FrameQueue_H
#define FrameQueue_H

// --------------------------------------------------------------------------------------
// Description:
//   A FIFO thread-safe queue to store frames between parser/decoders/players. The 
//   FullPolicy template parameter is the type of a "full policy" - an object that 
//   reports if the queue is full. When the queue is full, its Push method blocks until
//   someone Pops elements from the queue. The queue can enter "complete" state. In
//   the complete state both Pop and Push methods become non-blocking: all Pushes are
//   ignored and Pop with an empty queue returns NULL.
//   The queue owns its elements and transfers ownership with Pop method.
// See Also:
//   MaxCountFullPolicy, MaxIntervalPolicy
// --------------------------------------------------------------------------------------
template <typename Frame, typename FullPolicy, typename Deleter = TrackableDelete<Frame>>
class FrameQueue
{
public:
	FrameQueue( FullPolicy fullPolicy, Deleter deleter = Deleter() )
		:m_mutex( "FrameQueue", "m_mutex" ),
		m_complete( false ),
		m_fullPolicy( fullPolicy ),
		m_deleter( deleter )
	{
	}

	~FrameQueue()
	{
	}

	void Push( Frame* packet )
	{
		while( true )
		{
			m_mutex.Acquire();
			if( m_complete )
			{
				m_mutex.Release();
				m_deleter( packet );
				break;
			}
			if( !m_fullPolicy.IsFull( *this ) )
			{
				m_queue.push_back( std::unique_ptr<Frame, Deleter>( packet, m_deleter ) );
				m_mutex.Release();
				break;
			}
			m_mutex.Release();
			m_changed.Wait();
		}
		m_changed.Signal();
	}

	std::unique_ptr<Frame, Deleter> Pop()
	{
		while( true )
		{
			m_mutex.Acquire();
			if( m_queue.empty() )
			{
				if( m_complete )
				{
					m_mutex.Release();
					return nullptr;
				}
			}
			else
			{
				auto frame = std::move( m_queue.front() );
				m_queue.pop_front();
				m_mutex.Release();
				m_changed.Signal();
				return frame;
			}
			m_mutex.Release();
			m_changed.Wait();
		}
	}

	Frame* Peek( size_t index = 0 ) const
	{
		Frame* front = nullptr;
		m_mutex.Acquire();
		if( index < m_queue.size() )
		{
			front = m_queue[index].get();
		}
		m_mutex.Release();
		return front;
	}

	Frame* PeekLast() const
	{
		Frame* front = nullptr;
		m_mutex.Acquire();
		if( !m_queue.empty() )
		{
			front = m_queue.back().get();
		}
		m_mutex.Release();
		return front;
	}

	size_t Size() const
	{
		m_mutex.Acquire();
		size_t size = m_queue.size();
		m_mutex.Release();
		return size;
	}

	bool IsFull() const
	{
		m_mutex.Acquire();
		bool isFull = m_complete || m_fullPolicy.IsFull( *this );
		m_mutex.Release();
		return isFull;
	}

	void SetComplete()
	{
		m_mutex.Acquire();
		m_complete = true;
		m_mutex.Release();
		m_changed.Signal();
	}

	bool IsComplete() const
	{
		CcpAutoMutex lock( m_mutex );
		return m_complete;
	}

	FullPolicy& GetFullPolicy()
	{
		return m_fullPolicy;
	}
private:
	std::deque<std::unique_ptr<Frame, Deleter>> m_queue;
	mutable CcpMutex m_mutex;
	CcpSemaphore m_changed;
	bool m_complete;
	FullPolicy m_fullPolicy;
	Deleter m_deleter;
};


// --------------------------------------------------------------------------------------
// Description:
//   A full policy for FrameQueue. Maintains a limit on the number of items in the queue.
// See Also:
//   FrameQueue
// --------------------------------------------------------------------------------------
class MaxCountFullPolicy
{
public:
	MaxCountFullPolicy( size_t maxCount )
		:m_maxCount( maxCount )
	{
	}

	void SetMaxCount( size_t maxCount )
	{
		m_maxCount = maxCount;
	}

	template <typename FrameQueue>
	bool IsFull( const FrameQueue& frameQueue ) const
	{
		return frameQueue.Size() >= m_maxCount;
	}
private:
	size_t m_maxCount;
};


// --------------------------------------------------------------------------------------
// Description:
//   A full policy for FrameQueue. Maintains a limit on the length (actual time) of 
//   frames in the queue.
// See Also:
//   FrameQueue
// --------------------------------------------------------------------------------------
class MaxIntervalPolicy
{
public:
	MaxIntervalPolicy( int64_t intervalNs )
		:m_intervalNs( intervalNs )
	{
	}

	template <typename FrameQueue>
	bool IsFull( const FrameQueue& frameQueue ) const
	{
		auto first = frameQueue.Peek();
		auto last = frameQueue.PeekLast();
		if( !first || !last )
		{
			return false;
		}
		return int64_t( last->GetTimeStamp() - first->GetTimeStamp() ) >= m_intervalNs;
	}
private:
	int64_t m_intervalNs;
};

#endif