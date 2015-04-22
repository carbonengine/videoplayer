////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "WaveOutAudioSink.h"


BLUE_DEFINE_INTERFACE( IAudioSinkExposed );

#ifdef _WIN32

BLUE_DEFINE( WaveOutAudioSink );

const Be::ClassInfo* WaveOutAudioSink::ExposeToBlue()
{
    EXPOSURE_BEGIN( WaveOutAudioSink, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( IAudioSinkExposed )
		MAP_INTERFACE( WaveOutAudioSink )
	EXPOSURE_END()
}

#endif