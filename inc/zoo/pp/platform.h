#ifndef ZOO_PLATFORM_MACROS_H
#define ZOO_PLATFORM_MACROS_H

#ifdef __AVX2__
#define ZOO_CONFIGURED_TO_USE_AVX 1
#else
#define ZOO_CONFIGURED_TO_USE_AVX 0
#endif

#ifdef _MSC_VER
#define MSVC_EMPTY_BASES __declspec(empty_bases)
#else
#define MSVC_EMPTY_BASES
#endif

#endif
