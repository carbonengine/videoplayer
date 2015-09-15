#include "StdAfx.h"
#ifdef WITH_TESTS
#include "VideoController.h"
#include "TestUtilities.h"


TEST( VideoController, ControllerStartsParsingMetadata )
{
	TestMemoryStream stream( nullptr, nullptr );
	VideoController controller( &stream, nullptr );
	EXPECT_EQ( VideoController::PARSING_METADATA, controller.GetState() );
}

TEST( VideoController, ControllerHasNoErrorsOnCreation )
{
	TestMemoryStream stream( nullptr, nullptr );
	VideoController controller( &stream, nullptr );
	VideoController::Errors errors;
	controller.GetErrors( errors );
    // sometimes the parser may be able to start reading our invalid stream, so
    // we can't test parser state here
	//EXPECT_EQ( PARSER_ERROR_OK, errors.parserError );
	EXPECT_EQ( DECODER_ERROR_OK, errors.audioDecoderError );
	EXPECT_EQ( DECODER_ERROR_OK, errors.videoDecoderError );
}

TEST( VideoController, ControllerStopsOnInvalidStream )
{
	TestMemoryStream stream( nullptr, nullptr );
	VideoController controller( &stream, nullptr );
	for( int i = 0; i < 3000; ++i )
	{
		controller.Update();
		if( controller.GetState() > VideoController::PARSING_METADATA )
		{
			break;
		}
		CcpThreadSleep( 1 );
	}
	EXPECT_EQ( VideoController::DONE, controller.GetState() );
}

TEST( VideoController, ControllerReportsErrorsOnInvalidStream )
{
	TestMemoryStream stream( nullptr, nullptr );
	VideoController controller( &stream, nullptr );
	for( int i = 0; i < 3000; ++i )
	{
		controller.Update();
		if( controller.GetState() > VideoController::PARSING_METADATA )
		{
			break;
		}
		CcpThreadSleep( 1 );
	}
	EXPECT_EQ( VideoController::DONE, controller.GetState() );
	VideoController::Errors errors;
	controller.GetErrors( errors );
	EXPECT_EQ( PARSER_ERROR_INVALID_STREAM, errors.parserError );
}

#endif