#include "stdint.h"
#include <cstdlib>

uint32_t parse_eight_digits_swar(const char *chars);
uint32_t lemire_as_zoo_swar(const char *chars);

namespace zoo {

std::size_t c_strLength(const char *s);
std::size_t c_strLength_natural(const char *s);
std::size_t c_strLength_manualComparison(const char *s);

#if defined(__AVX2__)
std::size_t avx2_strlen(const char* str);
#endif

}

std::size_t
STRLEN_old (const char *str);
