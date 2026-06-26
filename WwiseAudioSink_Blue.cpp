// Copyright © 2020 CCP ehf.

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
		MAP_PROPERTY( 
			"volume", 
			GetVolume, 
			SetVolume,
			"The volume of the video that's playing. Must be a float between 0 and 1."
		);
	EXPOSURE_END()
}
