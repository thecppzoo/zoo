#ifndef ZOO_PLATFORM_MACROS_H
#define ZOO_PLATFORM_MACROS_H

#ifdef __AVX2__
#define ZOO_CONFIGURED_TO_USE_AVX() 1
#else
#define ZOO_CONFIGURED_TO_USE_AVX() 0
#endif

#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && (defined(__aarch64__) || defined(_M_ARM64))
#define ZOO_CONFIGURED_TO_USE_NEON() 1
#else
#define ZOO_CONFIGURED_TO_USE_NEON() 0
#endif

#ifdef __BMI2__
#define ZOO_CONFIGURED_TO_USE_BMI() 1
#else
#define ZOO_CONFIGURED_TO_USE_BMI() 0
#endif

#ifdef _MSC_VER
#define ZOO_WINDOWS_BUILD() 1
#define MSVC_EMPTY_BASES __declspec(empty_bases)
#else
#define ZOO_WINDOWS_BUILD() 0
#define MSVC_EMPTY_BASES
#endif

#endif
