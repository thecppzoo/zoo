#include "zoo/Any/VTablePolicy.h"
#include "zoo/AnyContainer.h"
#include "zoo/AlignedStorage.h"

#include <utility>
#include <type_traits>

namespace zoo {

struct CanonicalVTableAny {
    AlignedStorageFor<typename CanonicalVTablePolicy::MemoryLayout> space_;
};

static_assert(impl::HasCopy<CanonicalVTablePolicy::MemoryLayout>::value);

}

#include "catch2/catch.hpp"

TEST_CASE("Ligtests") {
    //using VTA = zoo::AnyContainer<zoo::CanonicalVTablePolicy>;
    using Policy = zoo::Policy<zoo::Destroy, zoo::Move, zoo::Copy>;
    using VTA = zoo::AnyContainer<Policy>;
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

            TracesMoves(const TracesMoves &) {
                throw std::runtime_error("unreachable called");
            }

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
