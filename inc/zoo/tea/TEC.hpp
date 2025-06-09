
#include "zoo/AlignedStorage.h"
#include "zoo/pp/platform.h"
#include "zoo/tea/Traits.h"
#include "zoo/tea/VTablePointerWrapper.h"
#include "zoo/utility.h"

#include "zoo/meta/copy_and_move_abilities.h"
#include <type_traits>

namespace zoo::tea {

namespace detail {

template<typename T, typename = void>
struct HasVTableMember: std::false_type {};

template<typename T>
struct HasVTableMember<T, std::void_t<typename T::VTable>>: std::true_type {};

struct Boo {
    using VTable = int;
};

static_assert(!HasVTableMember<void>::value);
static_assert(HasVTableMember<Boo>::value);

} // detail


template<typename Policy_>
struct TEC {
    using Policy = Policy_;
    static_assert(zoo::tea::detail::HasVTableMember<Policy>::value);

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


 //    void move(


    ~TEC() {
        space_.template as<DefaultManager>()->destroy();
    }

};


template<typename Policy>
struct CopyableTEC: TEC<Policy> {

};


struct FoolishPolicy {
    using VTable = int;
    struct MemoryLayout {};
};


static_assert(!std::is_move_constructible_v<TEC<FoolishPolicy>>);

}
