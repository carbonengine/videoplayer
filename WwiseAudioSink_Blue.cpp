////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Eric Nielsen
// Created:		August 2020
// Copyright:	CCP 2020
//

#include "StdAfx.h"
#include "WwiseAudioSink.h"

BLUE_DEFINE( WwiseAudioSink );
BLUE_DEFINE_INTERFACE( IAudioInputMgr );
BLUE_DEFINE_INTERFACE( IAudioInputSink );

const Be::ClassInfo* WwiseAudioSink::ExposeToBlue()
{
    EXPOSURE_BEGIN( WwiseAudioSink, "" )

		MAP_INTERFACE( IAudioInputSink )
		MAP_INTERFACE( IAudioSinkExposed )
		MAP_INTERFACE( WwiseAudioSink )

		MAP_METHOD_AND_WRAP( 
			"__init__", 
			Create, 
			"Creates an audio sink for Wwise.\n"
			":param audioInputMgr: Instance of an audio2.AudioInputMgr" 
		);
	EXPOSURE_END()
}
