#ifndef ZOO_VALID_EXPRESSION
#define ZOO_VALID_EXPRESSION

#include <utility>

#define VALID_EXPRESSION(name, type, ...)\
    template<typename type, typename = void>\
    struct name: std::false_type {};\
    template<typename type>\
    struct name<type, std::void_t<decltype(__VA_ARGS__)>>: std::true_type {}

#endif
