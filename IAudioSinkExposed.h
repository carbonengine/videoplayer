////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef IAudioSinkExposed_H
#define IAudioSinkExposed_H

#include "IAudioSink.h"

// --------------------------------------------------------------------------------------
// Description:
//   A blue-exposed version of IAudioSink.
// See also:
//   IAudioSink
// --------------------------------------------------------------------------------------
BLUE_INTERFACE( IAudioSinkExposed ): public IRoot, public IAudioSink
{
};

#endif