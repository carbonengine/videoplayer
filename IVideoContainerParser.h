#pragma once
#ifndef IVideoContainerParser_H
#define IVideoContainerParser_H

#include "Metadata.h"

// --------------------------------------------------------------------------------------
// Description:
//   A base interface for video container parser. The task of the parser is to read data
//   from provided stream, parse it and fill audio and video queues with encoded frames.
// See also:
//   WebMParser
// --------------------------------------------------------------------------------------
struct IVideoContainerParser
{
	virtual ~IVideoContainerParser() = 0;
	virtual bool IsMetadataAvailable() const = 0;
	virtual const VideoMetadata& GetVideoMetadata() const = 0;
	virtual const AudioMetadata& GetAudioMetadata() const = 0;
	virtual void CompleteQueues() = 0;
	virtual uint64_t GetDuration() const = 0;
	virtual uint64_t GetDownloadedMediaTime() const = 0;

	virtual void Seek( uint64_t time ) = 0;

	virtual EncodedAudioFrameQueue* GetAudioQueue() = 0;
	virtual EncodedVideoFrameQueue* GetVideoQueue() = 0;
	virtual ParserError GetError() const = 0;
};

// --------------------------------------------------------------------------------------
// Description:
//   Creates a parser from the given stream.
// --------------------------------------------------------------------------------------
IVideoContainerParser* CreateVideoContainerParser( ICcpStream* videoStream, StreamType outputStreams, unsigned audioTrack = 0, bool looped = false );

#endif