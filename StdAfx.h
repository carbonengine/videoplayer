#define NOMINMAX

#include <BlueExposure.h>
#include <Blue.h>
#include <IBlueOS.h>
#include <IBluePersist.h>
#include <BlueStatistics.h>

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
