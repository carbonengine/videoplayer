// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#ifdef WITH_TESTS

int main( int argc, char **argv ) 
{
	::testing::InitGoogleTest( &argc, argv );
	int result = RUN_ALL_TESTS();
	return result;
}
#endif