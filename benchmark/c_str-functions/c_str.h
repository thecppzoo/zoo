#ifndef ZOO_ATOI_H
#define ZOO_ATOI_H

#include "zoo/swar/SWAR.h"
#include "zoo/pp/platform.h"

#include <cstdlib>

uint32_t parse_eight_digits_swar(const char *chars);
uint32_t lemire_as_zoo_swar(const char *chars) noexcept;

std::size_t spaces_glibc(const char *ptr);

namespace zoo {

std::size_t leadingSpacesCount(const char *) noexcept;

std::size_t c_strLength(const char *s);
std::size_t c_strLength_natural(const char *s);
int32_t c_strToI(const char *) noexcept;
int64_t c_strToL(const char *) noexcept;
int64_t c_strToL128(const char *) noexcept;

template<long long (*FUN)(const char *s)>
inline int compareAtol(const char *s) {
    auto
        from_stdlib = atoll(s),
        from_zoo = FUN(s);
    if(from_stdlib != from_zoo) {
        auto recalc = FUN(s);
        throw 0; }
    return from_stdlib;
}

#if ZOO_CONFIGURED_TO_USE_AVX()
std::size_t avx2_strlen(const char* str);
#endif

#if ZOO_CONFIGURED_TO_USE_NEON()
std::size_t neon_strlen(const char* str);
#endif

}

std::size_t
STRLEN_old (const char *str);

#endif