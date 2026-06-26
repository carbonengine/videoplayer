// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#ifdef WITH_TESTS
#include "WebMParser.h"
#include "TestUtilities.h"


TEST( WebMParser, ParserReportsInvalidStreamForEmptyStream )
{
	TestMemoryStream stream( nullptr, nullptr );
	WebMParser parser( &stream, STREAM_AUDIO_VIDEO );
	parser.WaitForMetadata();
	EXPECT_EQ( PARSER_ERROR_INVALID_STREAM, parser.GetError() );
}

TEST( WebMParser, InvalidParserDoesNotHaveVideoQueue )
{
	TestMemoryStream stream( nullptr, nullptr );
	WebMParser parser( &stream, STREAM_AUDIO_VIDEO );
	parser.WaitForMetadata();
	EXPECT_EQ( nullptr, parser.GetVideoQueue() );
}

TEST( WebMParser, InvalidParserDoesNotHaveAudioQueue )
{
	TestMemoryStream stream( nullptr, nullptr );
	WebMParser parser( &stream, STREAM_AUDIO_VIDEO );
	parser.WaitForMetadata();
	EXPECT_EQ( nullptr, parser.GetAudioQueue() );
}

TEST( WebMParser, ParserReportsMetadataUnavailableBeforeItIsRead )
{
	TestMemoryStream stream( nullptr, nullptr );
	stream.StallOperations();

	WebMParser parser( &stream, STREAM_AUDIO_VIDEO );
	EXPECT_FALSE( parser.IsMetadataAvailable() );
	stream.ResumeOperations();
}

TEST( WebMParser, ParserReportsMetadataUnavailableForInvalidStream )
{
	TestMemoryStream stream( nullptr, nullptr );

	WebMParser parser( &stream, STREAM_AUDIO_VIDEO );
	parser.WaitForMetadata();
	EXPECT_FALSE( parser.IsMetadataAvailable() );
}

#endif