#include "AnyCallableGeneric.h"

template<typename S>
using Functor = zoo::AnyCallable<zoo::AnyContainer<zoo::CanonicalPolicy>, S>;

TEST_CASE("function", "[any][type-erasure][functional]") {
    CallableTests<Functor>::execute();
}

struct MO {
    MO() = default;
    MO(MO &&) = default;
    MO(const MO &) = delete;
};

MO::MO(MO &&) noexcept {};

struct MOD: MO {
    MOD() = default;
    MOD(MOD &&) = default;
    MOD(const MOD &) = default;
    MOD(std::nullptr_t);
};

MOD::MOD(std::nullptr_t) {}
