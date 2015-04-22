////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef VideoPlayer_H
#define VideoPlayer_H

#include "VideoController.h"
#include "trinity/Include/ITriTextureRes.h"
#include "VideoPlayerResult.h"

BLUE_DECLARE_INTERFACE( IAudioSinkExposed );


// --------------------------------------------------------------------------------------
// Description:
//   A blue-exposed class that encapsulated video controller.
// See also:
//   VideoController
// --------------------------------------------------------------------------------------
BLUE_CLASS( VideoPlayer ): public INotify, public IBlueEvents
{
public:
	EXPOSE_TO_BLUE();

	VideoPlayer(  IRoot* lockobj = nullptr );
	~VideoPlayer();

	Be::BlueStdResult Create( IBlueStream* stream, IAudioSinkExposed* audioSink );
	VideoPlayerResult Update();
	VideoController::State GetState() const;
	void Pause();
	void Resume();
	bool IsPaused() const;

	VideoPlayerResult GetVideoInfo( std::map<std::string, uint32_t>& metadata );
	VideoPlayerResult Validate();
	void ClearTextures();

	virtual bool OnModified( Be::Var* value );

	virtual void OnTick( Be::Time realTime, Be::Time simTime, void* cookie );

private:
	std::unique_ptr<VideoController, TrackableDelete<VideoController>> m_video;
	IBlueStreamPtr m_stream;
	IAudioSinkExposedPtr m_audioSink;
	ITriTextureResPtr m_yChannel;
	ITriTextureResPtr m_uChannel;
	ITriTextureResPtr m_vChannel;
	uint64_t m_lastUpdatedTimeStamp;
	VideoController::State m_lastState;
	VideoPlayerResult m_lastError;
	BlueScriptCallback m_onStateChange;
	BlueScriptCallback m_onError;
};

TYPEDEF_BLUECLASS( VideoPlayer );

#endif