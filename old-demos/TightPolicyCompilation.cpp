#include "TightPolicy.h"

#ifdef MALFORMED_ON_ARRAY_TOO_LARGE_FOR_STRING7
String7 convert() {
    return "Hello World";
}
#endif

#ifdef MALFORMED_ON_CONSTRUCTING_STRING7_FROM_ONLY_POINTER_TO_CHAR
String7 convert() {
    auto hello = "Hello World";
    return hello;
}
#endif
