// Copyright © 2015 CCP ehf.

#pragma once
#ifndef VideoPlayerResult_H
#define VideoPlayerResult_H

#include "VideoController.h"

template <>
struct Be::Result<VideoController::Errors>
{
	Result();
	Result( const VideoController::Errors& result );
	Result( const char* message );
	operator bool() const;

	VideoController::Errors m_result;
	const char* m_message;
};

template<> inline bool BeIsSuccess( const Be::Result<VideoController::Errors>& result )
{
	return result;
}

const char* BeGetErrorMessage( const Be::Result<VideoController::Errors>& result );

BLUE_DECLARE_GET_EXCEPTION( Be::Result<VideoController::Errors> );

typedef Be::Result<VideoController::Errors> VideoPlayerResult;
typedef Be::BlueWithStdResult<VideoPlayerResult>::type StdOrVideoPlayerResult;

#endif