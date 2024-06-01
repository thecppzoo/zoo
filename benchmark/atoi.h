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

inline int compareAtoi(const char *s) {
    auto
        from_stdlib = atoi(s),
        from_zoo = c_strToI(s);
    if(from_stdlib != from_zoo) { throw 0; }
    return from_stdlib;
}

inline int compareAtol(const char *s) {
    auto
        from_stdlib = atoll(s),
        from_zoo = c_strToL(s);
    if(from_stdlib != from_zoo) {
        auto recalc = c_strToL(s);
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
