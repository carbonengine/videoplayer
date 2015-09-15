#define NOMINMAX

#include "BlueExposure/include/BlueExposure.h"
#include "blue/Include/Blue.h"
#include "blue/Include/IBlueOS.h"
#include "blue/Include/IBluePersist.h"
#include "blue/Include/BlueStatistics.h"

#ifdef _PY_PORT_CTYPE_UTF8_ISSUE
#undef isalnum
#undef isalpha
#undef islower
#undef isspace
#undef isupper
#undef tolower
#undef toupper
#endif

#ifdef WITH_TESTS
#include "gtest/gtest.h"
#endif
