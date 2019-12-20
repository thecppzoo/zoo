#include "zoo/Any/VTablePolicy.h"
#include "zoo/AnyContainer.h"
#include "zoo/AlignedStorage.h"

#include <utility>
#include <type_traits>

namespace zoo {

struct VTable: Destroy::VTableEntry, Move::VTableEntry {};

struct VTableHolder {
    const VTable *ptr_;

    template<typename Affordance>
    const typename Affordance::VTableEntry *vTable() const noexcept {
        return static_cast<const typename Affordance::VTableEntry *>(ptr_);
    }

    VTableHolder(const VTable *p): ptr_(p) {}
};

struct PseudoContainer:
    VTableHolder,
    Destroy::Mixin<PseudoContainer>,
    Move::Mixin<PseudoContainer>
{
    AlignedStorageFor<void *> space_;

    static void moveVTable(void *to, void *from) noexcept {
        static_cast<PseudoContainer *>(to)->ptr_ =
            static_cast<PseudoContainer *>(from)->ptr_;
    }

    constexpr static inline VTable Default = {
        Destroy::Default<PseudoContainer>,
        Move::Default<PseudoContainer>
    };

    PseudoContainer(): VTableHolder(&Default) {}

    using VTableHolder::VTableHolder;
};

template<typename V>
struct ByValue: PseudoContainer {
    V *value() noexcept { return space_.template as<V>(); }

    static void destructor(void *p) noexcept {
        static_cast<ByValue *>(p)->value()->~V();
    }

    static void move(void *to, void *from) noexcept {
        auto downcast = static_cast<ByValue *>(from);
        new(to) ByValue(std::move(*downcast->value()));
    }

    constexpr static inline VTable Operations = {
        Destroy::Operation<ByValue>,
        Move::Operation<ByValue>
    };

    template<typename... Args>
    ByValue(Args &&...args): PseudoContainer(&Operations) {
        this->space_.template build<V>(std::forward<Args>(args)...);
    }
};

template<typename V>
struct ByReference: PseudoContainer {
    V *&pValue() noexcept { return *space_.template as<V *>(); }
    V *value() noexcept { return pValue(); }

    static void destructor(void *p) noexcept {
        auto ptr = static_cast<ByReference *>(p)->pValue();
        if(ptr) { delete ptr; }
    }

    static void move(void *to, void *from) noexcept {
        auto downcast = static_cast<ByReference *>(from);
        new(to) ByReference(downcast);
        downcast->pValue() = nullptr;
    }

    constexpr static inline VTable Operations = {
        Destroy::Operation<ByReference>,
        Move::Operation<ByReference>
    };

    ByReference(ByReference *source): PseudoContainer(&Operations) {
        pValue() = source->pValue();
    }

    template<typename... Args>
    ByReference(Args &&...args): PseudoContainer(&Operations) {
        pValue() = new V(std::forward<Args>(args)...);
    }
};

template<int S, int A, typename V>
struct BuilderDecider {
    constexpr static auto NoThrowMovable =
        std::is_nothrow_move_constructible_v<V>;
    constexpr static auto AlignmentSuitable =
        A % alignof(V) == 0;
    constexpr static auto SmallBufferSuitable =
        sizeof(V) <= S && AlignmentSuitable && NoThrowMovable;
};

struct Policy {
    using MemoryLayout = PseudoContainer;

    template<typename V>
    using Builder =
        std::conditional_t<
            BuilderDecider<8, 8, V>::SmallBufferSuitable,
            ByValue<V>,
            ByReference<V>
        >;
};

struct PseudoAny {
    AlignedStorageFor<typename Policy::MemoryLayout> space_;
};

}

#include "catch2/catch.hpp"

TEST_CASE("Ligtests") {
    using VTA = zoo::AnyContainer<zoo::Policy>;
    static_assert(std::is_default_constructible_v<VTA>);
    static_assert(std::is_nothrow_move_constructible_v<VTA>);

    SECTION("Destruction") {
        SECTION("Destruction of default constructed") {
            VTA a;
        }
        int trace = 0;

        struct TracesDestruction {
            int &trace_;

            TracesDestruction(int &trace): trace_(trace) {}
            ~TracesDestruction() { trace_ = 1; }
        };

        SECTION("By value") {
            REQUIRE(0 == trace);
            {
                VTA a(std::in_place_type<TracesDestruction>, trace);
            }
            REQUIRE(1 == trace);
        }
        SECTION("By reference") {
            struct Large: TracesDestruction {
                using TracesDestruction::TracesDestruction;
                double whatever[3];
            };
            REQUIRE(0 == trace);
            {
                VTA a(std::in_place_type<Large>, trace);
            }
            REQUIRE(1 == trace);
        }
    }
    SECTION("Moving") {
        struct TracesMoves {
            int *resource_;

            TracesMoves(int *p): resource_(p) {}

            TracesMoves(TracesMoves &&m) noexcept: resource_(m.resource_) {
                m.resource_ = nullptr;
            }

            ~TracesMoves() {
                if(resource_) { *resource_ = 0; }
            }
        };
        int something = 1;
        SECTION("By Value") {
            {
                VTA a;
                {
                    VTA tmp(std::in_place_type<TracesMoves>, &something);
                    REQUIRE(1 == something);
                    a = std::move(tmp);
                }
                REQUIRE(1 == something); // tmp if moved does not refer to something
            }
            REQUIRE(0 == something); // a referred to something, cleared
        }
        SECTION("By Reference") {
            struct Big {
                void *myself_;
                int *padding_;
                Big(): myself_(this) {}
            };
            VTA a(std::in_place_type<Big>);
            auto originalPlace = a.state<Big>()->myself_;
            VTA other(std::move(a));
            a.reset();
            REQUIRE(other.state<Big>()->myself_ == originalPlace);
        }
    }
}
