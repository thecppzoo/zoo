#include "zoo/swar/SWAR.h"
#include "zoo/pp/platform.h"

#include <cstdlib>

uint32_t parse_eight_digits_swar(const char *chars);
uint32_t lemire_as_zoo_swar(const char *chars);

std::size_t spaces_glibc(const char *ptr);

namespace zoo {

std::size_t leadingSpacesCount(const char *) noexcept;

std::size_t c_strLength(const char *s);
std::size_t c_strLength_natural(const char *s);

#if ZOO_CONFIGURED_TO_USE_AVX()
std::size_t avx2_strlen(const char* str);
#endif

#if ZOO_CONFIGURED_TO_USE_NEON()
std::size_t neon_strlen(const char* str);
#endif

}

std::size_t
STRLEN_old (const char *str);
