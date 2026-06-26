// Copyright © 2015 CCP ehf.

#pragma once
#ifndef Timer_H
#define Timer_H

// --------------------------------------------------------------------------------------
// Description:
//   A timer with pause/resume functionality. Used by VideoController.
// --------------------------------------------------------------------------------------
class Timer
{
public:
	Timer();

	void Start();
	void Pause();
	void Resume();
	uint64_t GetTime();
	bool IsRunning() const;
	bool IsPaused() const;
private:
	uint64_t m_startTime;
	uint64_t m_frequency;
	uint64_t m_accumulatedTime;
	bool m_running;
	int32_t m_pauseCount;
};

#endif