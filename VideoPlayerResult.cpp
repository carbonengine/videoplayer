////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "VideoPlayerResult.h"

namespace Be
{

Result<VideoController::Errors>::Result()
	:m_message( nullptr )
{
	m_result.parserError = PARSER_ERROR_OK;
	m_result.audioDecoderError = DECODER_ERROR_OK;
	m_result.videoDecoderError = DECODER_ERROR_OK;
}

Result<VideoController::Errors>::Result( const VideoController::Errors& result )
	:m_result( result ),
	m_message( nullptr )
{
}

Result<VideoController::Errors>::Result( const char* message )
	:m_message( message )
{
}

Result<VideoController::Errors>::operator bool() const
{
	return m_message == nullptr && m_result;
}

}

const char* BeGetErrorMessage( const Be::Result<VideoController::Errors>& result )
{
	if( result.m_message )
	{
		return result.m_message;
	}
	switch( result.m_result.parserError )
	{
	case PARSER_ERROR_OK:
		break;
	case PARSER_ERROR_INVALID_STREAM:
		return "video stream is invalid (could not read metadata)";
	case PARSER_ERROR_INVALID_DATA:
		return "video stream is corrupt";
	default:
		return "unexpected error";
	}
	switch( result.m_result.audioDecoderError )
	{
	case DECODER_ERROR_OK:
		break;
	case DECODER_ERROR_UNSUPPORTED_CODEC:
		return "audio codec is not supported";
	default:
		return "unexpected error";
	}
	switch( result.m_result.videoDecoderError )
	{
	case DECODER_ERROR_OK:
		break;
	case DECODER_ERROR_UNSUPPORTED_CODEC:
		return "video codec is not supported";
	default:
		return "unexpected error";
	}
	return "";
}
