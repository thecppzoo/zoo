#include <new>
#include <typeinfo>

namespace zoo {

struct Destroy {
    struct VTableEntry { void (*dp)(void *) noexcept; };

    static void noOp(void *) noexcept {}

    template<typename>
    constexpr static inline VTableEntry Default = { noOp };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::destructor };

    template<typename Container>
    struct Mixin {
        void destroy() noexcept {
            auto downcast = static_cast<Container *>(this);
            downcast->template vTable<Destroy>()->dp(downcast);
        };
    };
};

struct Move {
    struct VTableEntry { void (*mp)(void *, void *) noexcept; };

    template<typename Container>
    constexpr static inline VTableEntry Default = { Container::moveVTable };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::move };

    template<typename Container>
    struct Mixin {
        void move(void *to) noexcept {
            auto downcast = static_cast<Container *>(this);
            downcast->template vTable<Move>()->mp(to, downcast);
        };
    };
};

}