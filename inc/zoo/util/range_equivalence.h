#ifndef ZOO_RANGE_EQUIVALENCE
#define ZOO_RANGE_EQUIVALENCE

#include <zoo/traits/is_container.h>

namespace zoo {

template<typename C1, typename C2>
auto operator==(const C1 &l, const C2 &r)
-> std::enable_if_t<
    zoo::is_container_v<C1> and
        zoo::is_container_v<C2>,
    bool
>
{
    auto lb{cbegin(l)}, le{cend(l)};
    auto rb{cbegin(r)}, re{cend(r)};
    for(;;++lb, ++rb){
        if(lb == le) { return rb == re; } // termination at the same time
        if(rb == re) { return false; } // r has fewer elements
        if(not(*lb == *rb)) { return false; }
    }
    return true;
}

template<typename C1, typename C2>
auto weaklySame(const C1 &l, const C2 &r)
-> std::enable_if_t<
    zoo::is_container_v<C1> and
        zoo::is_container_v<C2>,
    bool
>
{
    auto lb{cbegin(l)}, le{cend(l)};
    auto rb{cbegin(r)}, re{cend(r)};
    for(;;++lb, ++rb){
        if(lb == le) { return rb == re; } // termination at the same time
        if(rb == re) { return false; } // r has fewer elements
        if(*lb < *rb || *rb < *lb) { return false; }
    }
    return true;
}

}

#endif
