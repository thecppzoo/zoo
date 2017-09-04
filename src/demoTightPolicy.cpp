#include "TightPolicy.h"

static_assert(!is_stringy_type<int>::value, "");
static_assert(is_stringy_type<std::string>::value, "");

struct has_chars {
    char spc[8];
};

static_assert(is_stringy_type<decltype(has_chars::spc)>::value, "");
