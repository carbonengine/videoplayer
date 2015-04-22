#include "StdAfx.h"
#include "IVideoContainerParser.h"
#include "WebMParser.h"

IVideoContainerParser::~IVideoContainerParser()
{
}

IVideoContainerParser* CreateVideoContainerParser( ICcpStream* videoStream, StreamType outputStreams )
{
	return CCP_NEW( "CreateVideoContainerParser/WebMParser" ) WebMParser( videoStream, outputStreams );
}
