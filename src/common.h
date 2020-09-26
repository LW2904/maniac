#pragma once

#ifdef _WIN32
	#define ON_WINDOWS

	#include "Windows.h"

	#define __STDC_WANT_LIB_EXT1__ 1
#endif

#ifdef __linux__
	#define ON_LINUX
#endif

#include <cstdint>
#include <cstddef>

#ifdef DEBUG

	#define debug(...)\
		printf("[debug] [%s] ", __FUNCTION__ );\
		printf(__VA_ARGS__);\
		putchar('\n');\

  	#define debug_short(...)\
		printf("[debug] ");\
		printf(__VA_ARGS__);\
		putchar('\n');\

#else

	#define debug(...)\
		;\

  	#define debug_short(...)\
  		;\

#endif
