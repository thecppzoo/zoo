#ifndef ZOO_LESS
#define ZOO_LESS

namespace zoo {

struct Less {
    template<typename T1, typename T2>
    bool operator()(const T1 &l, const T2 &r) {
        return l < r;
    }
};

}

#endif
