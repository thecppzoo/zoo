#ifndef ZOO_PLATFORM_MACROS_H
#define ZOO_PLATFORM_MACROS_H

#ifdef _MSC_VER

#define MSVC_EMPTY_BASES __declspec(empty_bases)

#else

#define MSVC_EMPTY_BASES

#endif

#endif
