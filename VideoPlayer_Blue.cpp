////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "VideoPlayer.h"
#include "IAudioSinkExposed.h"
#include "BlueExposure/include/InterfaceDefinitions.cxx"

BLUE_STANDARD_MODULE_INIT( _videoplayer );



static const Be::VarChooser VideoControllerState[] =
{
	{ "UNINITIALIZED",		BeCast( VideoController::UNINITIALIZED ),		"Player has not been properly initialized" }, 
	{ "PARSING_METADATA",	BeCast( VideoController::PARSING_METADATA ),	"Player is receiving stream meta data" }, 
	{ "INITIAL_BUFFERING",	BeCast( VideoController::INITIAL_BUFFERING ),	"Player performs initial buffering of data" }, 
	{ "PLAYING",			BeCast( VideoController::PLAYING ),				"Player is playing video" }, 
	{ "BUFFERING",			BeCast( VideoController::BUFFERING ),			"Player is buffering" }, 
	{ "FINISHING_BUFFERING",BeCast( VideoController::FINISHING_BUFFERING ),	"Player performs after-buffering pause" }, 
	{ "DONE",				BeCast( VideoController::DONE ),				"Player has finished playing video" }, 
	{0}
};

BLUE_REGISTER_ENUM_EX( "State", VideoController::State, VideoControllerState, ENUM_REG_ENUM_OBJECT_ON_MODULE );


// TODO: why do I need this here?
BLUE_DEFINE_INTERFACE( ITriTextureRes );


BLUE_DEFINE( VideoPlayer );

const Be::ClassInfo* VideoPlayer::ExposeToBlue()
{
    EXPOSURE_BEGIN( VideoPlayer, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( VideoPlayer )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__", 
			Create, 
			1,
			"Initializes video player.\n\n"
			":param stream: opened input stream to the video (needs to be a blue stream)" );
		MAP_PROPERTY_READONLY( 
			"state", 
			GetState, 
			"Current state of the player (read only). Member of State enum" );
		MAP_PROPERTY_READONLY( 
			"is_paused", 
			IsPaused, 
			"Is the playback paused (read only)" );
		MAP_METHOD_AND_WRAP( 
			"pause", 
			Pause, 
			"Pauses playback" );
		MAP_METHOD_AND_WRAP( 
			"resume", 
			Resume, 
			"Resumes playback after pausing" );
		MAP_METHOD_AND_WRAP( 
			"get_video_info", 
			GetVideoInfo, 
			"" );
		MAP_METHOD_AND_WRAP( 
			"validate", 
			Validate, 
			"Raises exception if any errors have occured during playback" );
		MAP_METHOD_AND_WRAP( 
			"clear_textures", 
			ClearTextures, 
			"Clears channel textures to black" );
		MAP_ATTRIBUTE( 
			"y_channel", 
			m_yChannel, 
			"Y channel texture. Needs to be a valid trinity.TriTextureRes texture\n"
			"with the size matching video size", 
			Be::READWRITE | Be::NOTIFY );
		MAP_ATTRIBUTE( 
			"cu_channel", 
			m_uChannel, 
			"Cu channel texture. Needs to be a valid trinity.TriTextureRes texture\n"
			"with the size matching video size (half of the video size)", 
			Be::READWRITE | Be::NOTIFY );
		MAP_ATTRIBUTE( 
			"cv_channel", 
			m_vChannel, 
			"Cv channel texture. Needs to be a valid trinity.TriTextureRes texture\n"
			"with the size matching video size (half of the video size)", 
			Be::READWRITE | Be::NOTIFY );

		MAP_ATTRIBUTE( 
			"on_state_change", 
			m_onStateChange, 
			"Callback function that is called whenever video player state changes.\n"
			"The function is called with single parameter which is the player instance", 
			Be::READWRITE );
		MAP_ATTRIBUTE( 
			"on_error", 
			m_onError, 
			"Callback function that is called whenever error occurs during asyncronous playback.\n"
			"The function is called with single parameter which is the player instance", 
			Be::READWRITE );
	EXPOSURE_END()
}
