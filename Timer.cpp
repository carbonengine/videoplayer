// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Timer.h"


Timer::Timer()
	:m_startTime( 0 ),
	m_frequency( 0 ),
	m_accumulatedTime( 0 ),
	m_running( false ),
	m_pauseCount( 0 )
{
}

void Timer::Start()
{
	m_running = true;
	m_startTime = CcpGetTimestamp();
	m_frequency = CcpGetTimestampFrequency();
	m_accumulatedTime = 0;
}

void Timer::Pause()
{
	if( m_pauseCount == 0 )
	{
		m_accumulatedTime += CcpGetTimestamp() - m_startTime;
	}
	++m_pauseCount;
}

void Timer::Resume()
{
	--m_pauseCount;
	if( m_pauseCount == 0 )
	{
		m_startTime = CcpGetTimestamp();
	}
}

uint64_t Timer::GetTime()
{
	if( !m_running )
	{
		return 0;
	}
	if( m_pauseCount > 0 )
	{
		return  m_accumulatedTime * 1000000 / m_frequency * 1000;
	}
	auto now = CcpGetTimestamp();
	return ( ( now - m_startTime + m_accumulatedTime ) * 1000000 / m_frequency ) * 1000;
}

bool Timer::IsRunning() const
{
	return m_running;
}

bool Timer::IsPaused() const
{
	return m_pauseCount > 0;
}