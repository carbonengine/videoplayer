////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef VideoController_H
#define VideoController_H

#include "IVideoContainerParser.h"
#include "IAudioDecoder.h"
#include "IVideoDecoder.h"
#include "IAudioSink.h"
#include "Timer.h"


// --------------------------------------------------------------------------------------
// Description:
//   A video controller orchestrates video playback. Its main method Update needs to be
//   called regularly to pump video playback. The controller instantiates the video 
//   container parser and audio/video decoders and tries to keep audio and video playback
//   in sync.
// --------------------------------------------------------------------------------------
class VideoController
{
public:
	VideoController( ICcpStream* stream, IAudioSink* audioSink );
	~VideoController();

	enum State
	{
		// the controller has not been properly initialized
		UNINITIALIZED,
		// waiting for video container parser to read stream metadata
		PARSING_METADATA,
		// performing initial buffering, filling encoded frame queues
		INITIAL_BUFFERING,
		// playing
		PLAYING,
		// during-playback-buffering (if the controller ran out of frames to present)
		BUFFERING,
		// a follow-up to BUFFERING state to prevent frequent switches between PLAYING and BUFFERING
		FINISHING_BUFFERING,
		// playback is finished
		DONE,
	};

	struct Errors
	{
		Errors()
			:parserError( PARSER_ERROR_OK ),
			audioDecoderError( DECODER_ERROR_OK ),
			videoDecoderError( DECODER_ERROR_OK )
		{
		}

		operator bool() const
		{
			return parserError == PARSER_ERROR_OK && audioDecoderError == DECODER_ERROR_OK && videoDecoderError == DECODER_ERROR_OK;
		}

		ParserError parserError;
		DecoderError audioDecoderError;
		DecoderError videoDecoderError;
	};

	void Update();
	VideoFrame* GetVideoFrame();
	State GetState() const { return m_state; }
	void GetErrors( Errors& errors );
	IVideoContainerParser& GetParser();
	uint64_t GetMediaTime();
	
	void Pause();
	void Resume();
	bool IsPaused() const;
private:
	bool NeedsBuffering() const;
	void RemoveExpiredFrames();
	bool IsDone() const;

	// current state
	State m_state;
	// timer for FINISHING_BUFFERING state
	Timer m_bufferingTimer;

	std::unique_ptr<IVideoContainerParser, TrackableDelete<IVideoContainerParser>> m_parser;
	std::unique_ptr<IVideoDecoder, TrackableDelete<IVideoDecoder>> m_videoDecoder;
	std::unique_ptr<IAudioDecoder, TrackableDelete<IAudioDecoder>> m_audioDecoder;
	IAudioSink* m_audioSink;
	// timer to use if audio sink is NULL (otherwise use audio time)
	Timer m_mediaTime;
	// if the playback is paused
	bool m_paused;
};

#endif