#ifndef ZOO_PLATFORM_MACROS_H
#define ZOO_PLATFORM_MACROS_H

#ifdef _MSC_VER

#define MSVC_EMPTY_BASES __declspec(empty_bases)

#else

#define MSVC_EMPTY_BASES

#define ZOO_CONFIGURED_TO_USE_AVX defined(__AVX2__) // TODO bring this in from CMake

#endif

#endif
