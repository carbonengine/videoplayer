// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "IVideoContainerParser.h"
#include "WebMParser.h"

IVideoContainerParser::~IVideoContainerParser()
{
}

IVideoContainerParser* CreateVideoContainerParser( ICcpStream* videoStream, StreamType outputStreams, unsigned audioTrack, bool looped )
{
	return CCP_NEW( "CreateVideoContainerParser/WebMParser" ) WebMParser( videoStream, outputStreams, audioTrack, looped );
}
