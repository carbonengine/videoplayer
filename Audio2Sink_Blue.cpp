////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "Audio2Sink.h"

BLUE_DEFINE( Audio2Sink );

const Be::ClassInfo* Audio2Sink::ExposeToBlue()
{
    EXPOSURE_BEGIN( Audio2Sink, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( IAudioSinkExposed )
		MAP_INTERFACE( Audio2Sink )

		MAP_METHOD_AND_WRAP( 
			"__init__", 
			Create, 
			"Creates an audio sink for audio2.\n"
			":param audio_function: The magic thing you get from audio2.GetDirectSoundPtr()\n"
			":param audio_position_function: The magic thing you get from audio2.GetStreamPositionPtr()\n"
			":param channel: Audio channel index" );
		MAP_ATTRIBUTE(
			"volume",
			m_volume,
			"Audio volume (from 0 to 1)",
			Be::READWRITE );
	EXPOSURE_END()
}
