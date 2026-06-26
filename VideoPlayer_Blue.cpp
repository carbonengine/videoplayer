// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "VideoPlayer.h"
#include "IAudioSinkExposed.h"

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
BLUE_DEFINE_INTERFACE( IBlueStream );


BLUE_DEFINE( VideoPlayer );

const Be::ClassInfo* VideoPlayer::ExposeToBlue()
{
    EXPOSURE_BEGIN( VideoPlayer, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( VideoPlayer )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__", 
			Create, 
			3,
			"Initializes video player.\n\n"
			":param stream: opened input stream to the video (needs to be a blue stream)\n"
			":param audio_sink: audio sink object (no audio output if omitted)\n" 
			":param audio_stream: audio track index (0 if omitted)\n" 
			":param looped: if the playback is looped (False by default)\n" );
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
			"seek", 
			Seek, 
			"Changes play position to a specified time\n"
			":param time: seek time (from video start) in nanoseconds" );
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
		MAP_PROPERTY( 
			"bgra_texture", 
			GetBgraTexture, 
			SetBgraTexture,
			"BGRA8 output texture. Needs to be a valid trinity.TriTextureRes texture\n"
			"with the size matching video size" );
		MAP_PROPERTY_READONLY(
			"media_time",
			GetMediaTime,
			"Media playback time in nanoseconds" );
		MAP_PROPERTY_READONLY(
			"duration",
			GetDuration,
			"Total media duration in nanoseconds" );
		MAP_PROPERTY_READONLY(
			"downloaded_media_time",
			GetDownloadedMediaTime,
			"Media time downloaded from input stream in nanoseconds" );
		MAP_ATTRIBUTE(
			"audio_sink",
			m_audioSink,
			"Audio sink object",
			Be::READ );

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
		MAP_ATTRIBUTE( 
			"on_create_textures", 
			m_onCreateTextures, 
			"Callback function that is called when video player needs to change output texture sizes.\n"
			"The function is called with arguments player_instance, (y_width, y_height), (uv_width, uv_height)", 
			Be::READWRITE );
	EXPOSURE_END()
}
