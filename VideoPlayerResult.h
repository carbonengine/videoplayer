////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef VideoPlayerResult_H
#define VideoPlayerResult_H

#include "VideoController.h"

namespace Be
{

template <>
struct Result<VideoController::Errors>
{
	Result();
	Result( const VideoController::Errors& result );
	Result( const char* message );
	operator bool() const;

	VideoController::Errors m_result;
	const char* m_message;
};

template<> inline bool IsSuccess( const Result<VideoController::Errors>& result )
{
	return result;
}

const char* GetErrorMessage( const Result<VideoController::Errors>& result );

BLUE_DECLARE_GET_EXCEPTION( Result<VideoController::Errors> );

}

typedef Be::Result<VideoController::Errors> VideoPlayerResult;
typedef Be::BlueWithStdResult<VideoPlayerResult>::type StdOrVideoPlayerResult;

#endif