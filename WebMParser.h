////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Filipp Pavlov
// Created:		April 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef WebMParser_H
#define WebMParser_H

#include "FrameQueue.h"
#include "Metadata.h"
#include "IVideoContainerParser.h"


struct nestegg;
struct nestegg_packet;


// --------------------------------------------------------------------------------------
// Description:
//   An encoded frame that stores a nestegg packet. Is emitted by WebMParser.
// See also:
//   WebMParser
// --------------------------------------------------------------------------------------
class NestEggFrame: public IEncodedFrame
{
public:
	explicit NestEggFrame( nestegg_packet* packet );
	~NestEggFrame();

	virtual size_t GetFrameCount();
	virtual uint64_t GetTimeStamp();
	virtual bool GetFrame( size_t index, uint8_t*& data, size_t& length );
private:
	nestegg_packet* m_packet;
};


// --------------------------------------------------------------------------------------
// Description:
//   An video container parser for WebM format.
// See also:
//   IVideoContainerParser
// --------------------------------------------------------------------------------------
class WebMParser: public IVideoContainerParser
{
public:
	WebMParser( ICcpStream* videoStream, StreamType outputStreams, unsigned audioTrack = 0 );
	~WebMParser();

	bool IsMetadataAvailable() const;
	void WaitForMetadata() const;
	const VideoMetadata& GetVideoMetadata() const;
	const AudioMetadata& GetAudioMetadata() const;
	void CompleteQueues();
	uint64_t GetDuration() const;
	uint64_t GetDownloadedMediaTime() const;

	EncodedAudioFrameQueue* GetAudioQueue();
	EncodedVideoFrameQueue* GetVideoQueue();
	ParserError GetError() const;
private:

	void ReadThread();
	unsigned FindTrack( int trackType, int trackIndex = 0 );

	VideoMetadata m_videoMetadata;
	AudioMetadata m_audioMetadata;
	bool m_metadataAvailable;
	mutable CcpMutex m_metadataMutex;
	mutable CcpSemaphore m_threadStarted;
	mutable CcpMutex m_downloadedMediaTimeMutex;

	std::unique_ptr<EncodedAudioFrameQueue, TrackableDelete<EncodedAudioFrameQueue>> m_audioQueue;
	std::unique_ptr<EncodedVideoFrameQueue, TrackableDelete<EncodedVideoFrameQueue>> m_videoQueue;
	nestegg* m_nestEgg;
	ICcpStream* m_videoStream;

	unsigned m_videoTrack;
	unsigned m_audioTrack;
	unsigned m_requestedAudioTrack;

	bool m_stopRequested;
	CcpThread m_parseThread;
	ParserError m_parserError;
	StreamType m_outputStreams;
	uint64_t m_duration;
	uint64_t m_downloadedMediaTime;
};

#endif