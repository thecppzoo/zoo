#pragma once

#ifndef NO_STANDARD_INCLUDES
#include <type_traits>
#include <utility>
#endif

namespace zoo::meta {
    template<typename T>
    struct InplaceType: std::false_type {};

    template<typename T>
    struct InplaceType<std::in_place_type_t<T>>: std::true_type {};
}
