#include "StdAfx.h"
#ifdef WITH_TESTS
#include "FrameQueue.h"

namespace
{

int32_t s_trackedObjectsAlive = 0;

struct TrackedObject
{
	TrackedObject()
	{
		++s_trackedObjectsAlive;
	}
	TrackedObject( const TrackedObject& )
	{
		++s_trackedObjectsAlive;
	}
	~TrackedObject()
	{
		--s_trackedObjectsAlive;
	}
};

class KillThreadOnExit
{
public:
	KillThreadOnExit( CcpThread&& thread )
		:m_thread( std::forward<CcpThread>( thread ) )
	{
	}

	~KillThreadOnExit()
	{
		if( m_thread.joinable() )
		{
			CcpKillThread( m_thread.native_handle() );
			m_thread.detach();
		}
	}
private:
	KillThreadOnExit( const KillThreadOnExit& );
	CcpThread m_thread;
};

template <typename Callable, typename Unblock>
class ThreadBlocksHelper
{
public:
	ThreadBlocksHelper( Callable& callable, Unblock& unblock )
		:m_callable( callable ),
        m_unblock( unblock ),
		m_semaphore( 0, 1 )
	{
		m_blocked = 1;
		m_thread = CcpThread( &ThreadBlocksHelper::ThreadFunc, this );
	}

	~ThreadBlocksHelper()
	{
        if( m_blocked )
        {
            m_unblock();
        }
        m_thread.join();
	}

	bool Wait( uint32_t timeoutMs )
	{
		return !m_semaphore.TimedWait( timeoutMs );
	}
private:
	void ThreadFunc()
	{
		m_callable();
		m_semaphore.Signal();
		m_blocked = 0;
	}
	Callable& m_callable;
    Unblock& m_unblock;
	CcpSemaphore m_semaphore;
	CcpThread m_thread;
	CcpAtomic<uint32_t> m_blocked;
};

template <typename Callable, typename Unblock>
bool ThreadBlocks( Callable callable, Unblock unblock, uint32_t timeoutInMs = 500 )
{
	ThreadBlocksHelper<Callable, Unblock> helper( callable, unblock );
	return helper.Wait( timeoutInMs );
}

}

TEST( FrameQueue, CanCreateQueue )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	EXPECT_EQ( 0, queue.Size() );
}

TEST( FrameQueue, PickingFromEmptyQueueReturnsNull )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	EXPECT_EQ( nullptr, queue.Peek() );
}

TEST( FrameQueue, PickingLastItemFromEmptyQueueReturnsNull )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	EXPECT_EQ( nullptr, queue.PeekLast() );
}

TEST( FrameQueue, PushingItemIntoQueueIncreasesSize )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	queue.Push( CCP_NEW( "test value" ) int( 11 ) );
	EXPECT_EQ( 1, queue.Size() );
	queue.Push( CCP_NEW( "test value" ) int( 22 ) );
	EXPECT_EQ( 2, queue.Size() );
}

TEST( FrameQueue, PickingItemInQueueReturnsFirstPushed )
{
	auto item1 = CCP_NEW( "test value" ) int( 11 );
	auto item2 = CCP_NEW( "test value" ) int( 11 );

	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	queue.Push( item1 );
	queue.Push( item2 );
	EXPECT_EQ( item1, queue.Peek() );
}

TEST( FrameQueue, PickLastReturnsFirstPushed )
{
	auto item1 = CCP_NEW( "test value" ) int( 11 );
	auto item2 = CCP_NEW( "test value" ) int( 11 );

	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	queue.Push( item1 );
	queue.Push( item2 );
	EXPECT_EQ( item2, queue.PeekLast() );
}

TEST( FrameQueue, QueueOwnsObjects )
{
	s_trackedObjectsAlive = 0;
	auto item1 = CCP_NEW( "test value" ) TrackedObject;
	EXPECT_EQ( 1, s_trackedObjectsAlive );
	{
		FrameQueue<TrackedObject, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
		queue.Push( item1 );
	}
	EXPECT_EQ( 0, s_trackedObjectsAlive );
}

TEST( FrameQueue, PopRemovesItemFromQueue )
{
	FrameQueue<TrackedObject, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	queue.Push( CCP_NEW( "test value" ) TrackedObject );
	queue.Pop();
	EXPECT_EQ( 0, queue.Size() );
}

TEST( FrameQueue, PopReturnsManagedPointer )
{
	s_trackedObjectsAlive = 0;
	FrameQueue<TrackedObject, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	queue.Push( CCP_NEW( "test value" ) TrackedObject );
	queue.Pop();
	EXPECT_EQ( 0, s_trackedObjectsAlive );
}

TEST( FrameQueue, PopReturnsFirstPushedItem )
{
	auto item1 = CCP_NEW( "test value" ) int( 11 );
	auto item2 = CCP_NEW( "test value" ) int( 11 );

	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 123 ) );
	queue.Push( item1 );
	queue.Push( item2 );
	EXPECT_EQ( item1, queue.Pop().get() );
}

TEST( FrameQueue, PushBlocksWhenCapacityIsReached )
{
	auto item1 = CCP_NEW( "test value" ) int( 11 );
	auto item2 = CCP_NEW( "test value" ) int( 11 );

	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 1 ) );
	queue.Push( item1 );
	EXPECT_TRUE( ThreadBlocks(
        [&]() {
            queue.Push( item2 );
        },
        [&]() {
            queue.Pop();
        }
      ) );
}

TEST( FrameQueue, PopBlocksWhenQueueIsEmpty )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 1 ) );
	EXPECT_TRUE( ThreadBlocks(
        [&]() {
            queue.Pop();
        },
        [&]() {
            queue.Push( CCP_NEW( "test value" ) int( 1 ) );
        }
        ) );
}

TEST( FrameQueue, PopUnblocksWhenQueueBecomesNonEmpty )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 1 ) );
	CcpAtomic<uint32_t> step( 0 );
	KillThreadOnExit t( CcpThread( [&] () {
		queue.Pop();
		step = 1;
	} ) );
	CcpThreadSleep( 10 );
	EXPECT_NE( 1, step );
	queue.Push( CCP_NEW( "test value" ) int( 1 ) );
	CcpThreadSleep( 10 );
	EXPECT_EQ( 1, step );
}

TEST( FrameQueue, PushAtCapacityUnblocksWhenItemIsPopped )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 1 ) );
	queue.Push( CCP_NEW( "test value" ) int( 1 ) );
	CcpAtomic<uint32_t> step( 0 );
	KillThreadOnExit t( CcpThread( [&] () {
		queue.Push( CCP_NEW( "test value" ) int( 1 ) );
		step = 1;
	} ) );
	CcpThreadSleep( 10 );
	EXPECT_NE( 1, step );
	queue.Pop();
	CcpThreadSleep( 10 );
	EXPECT_EQ( 1, step );
}

TEST( FrameQueue, QueueReportsNotFullWhenItIsNot )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 2 ) );
	EXPECT_FALSE( queue.IsFull() );
	queue.Push( CCP_NEW( "test value" ) int( 1 ) );
	EXPECT_FALSE( queue.IsFull() );
}

TEST( FrameQueue, QueueReportsFullWhenItIsFull )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 2 ) );
	queue.Push( CCP_NEW( "test value" ) int( 1 ) );
	queue.Push( CCP_NEW( "test value" ) int( 1 ) );
	EXPECT_TRUE( queue.IsFull() );
}

TEST( FrameQueue, IncompleteQueueReportsAsNotComplete )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 2 ) );
	EXPECT_FALSE( queue.IsComplete() );
}

TEST( FrameQueue, CompleteQueueReportsAsComplete )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 2 ) );
	queue.SetComplete();
	EXPECT_TRUE( queue.IsComplete() );
}

TEST( FrameQueue, PoppingFromEmptyCompleteQueueReturnsNull )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 2 ) );
	queue.SetComplete();
	int temp;
	int* result = &temp;
	EXPECT_FALSE( ThreadBlocks(
        [&]() {
            result = queue.Pop().get();
	    },
        [&]() {
        }
        ) );
	EXPECT_EQ( nullptr, result );
}

TEST( FrameQueue, PushingIntoFullCompleteQueueIsIgnored )
{
	FrameQueue<int, MaxCountFullPolicy> queue( MaxCountFullPolicy( 1 ) );
	queue.Push( CCP_NEW( "test value" ) int( 1 )  );
	queue.SetComplete();
	auto newValue = CCP_NEW( "test value" ) int( 1 );
	EXPECT_FALSE( ThreadBlocks(
        [&]() {
            queue.Push( newValue );
	    },
        [&]() {
        }
        ) );
	EXPECT_EQ( 1, queue.Size() );
}

#endif
