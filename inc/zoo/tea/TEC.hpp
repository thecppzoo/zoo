
#include "zoo/AlignedStorage.h"
#include "zoo/pp/platform.h"
#include "zoo/tea/Traits.h"
#include "zoo/utility.h"

#include "zoo/meta/copy_and_move_abilities.h"
#include <type_traits>

namespace zoo::tea {

template<typename Policy_>
struct TEC {
    using Policy = Policy_;

    TEC() = delete;
    TEC(const TEC &) = delete;

    using DefaultManager = typename Policy::MemoryLayout;

    AlignedStorageFor<DefaultManager> space_;

    using TokenType = void (TEC::*)();

    constexpr static inline TokenType Token = nullptr;

    template <typename Target, typename... Params>
    TEC(TokenType, Params &&...params)
        noexcept(
            std::is_nothrow_constructible_v<Target, Params &&...>
        )
    {
        space_.template build<Target>(std::forward<Params>(params)...);
    }

//     AnyContainerBase(AnyContainerBase &&moveable) noexcept:
//         SuperContainer(
//             SuperContainer::Token,
//             static_cast<SuperContainer &&>(moveable)
//         )
//     {
//         auto source = moveable.container();
//         source->move(container());
//     }


    void move(


    ~TEC() {
        space_.template as<DefaultManager>()->destroy();
    }

};

struct Foo {
    using MemoryLayout = void *;
};

static_assert(!std::is_move_constructible_v<TEC<Foo>>);

}
