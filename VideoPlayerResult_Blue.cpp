////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"
#include "VideoPlayerResult.h"


BLUE_DEFINE_EXCEPTION( VideoPlayerError, BlueStdRuntimeError );

// --------------------------------------------------------------------------------------
// Description:
//   Returns Python exception class for the given ALResult code.
// Arguments:
//   result - ALResult code
// Return value:
//   Python exception class appropriate for the given ALResult code
// --------------------------------------------------------------------------------------
BLUE_BEGIN_GET_EXCEPTION( VideoPlayerResult )
	return BLUE_GET_EXCEPTION( VideoPlayerError );
BLUE_END_GET_EXCEPTION()
