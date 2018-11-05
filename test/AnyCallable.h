#include <zoo/any.h>

static constexpr auto BigSize = 32;
using LargePolicy = zoo::RuntimePolymorphicAnyPolicy<BigSize, 8>;
using LargeTypeEraser = zoo::AnyContainer<LargePolicy>;

/// Uses Argument Dependent Lookup
struct BeforeAnyCallableEraserADL: LargeTypeEraser {};
void swap(BeforeAnyCallableEraserADL &e1, BeforeAnyCallableEraserADL &e2) noexcept {
    auto &upcasted = static_cast<LargeTypeEraser &>(e1);
    swap(upcasted, e2);
}

/// Uses the template with the policy as argument, works
struct BeforeAnyCallableEraserP: LargeTypeEraser {};
void swap(BeforeAnyCallableEraserP &e1, BeforeAnyCallableEraserP &e2) noexcept {
    zoo::swap<LargePolicy>(e1, e2);
}

/// Uses the template on the eraser as argument, won't use optimized overload
struct BeforeAnyCallableEraserT: LargeTypeEraser {};
void swap(BeforeAnyCallableEraserT &e1, BeforeAnyCallableEraserT &e2) noexcept {
    zoo::swap<LargeTypeEraser>(e1, e2);
}

struct AfterAnyCallableEraser: LargeTypeEraser {};

#include <zoo/function.h>

void swap(AfterAnyCallableEraser &e1, AfterAnyCallableEraser &e2) noexcept {
    zoo::swap<LargePolicy>(e1, e2);
}

struct TracesBase {
    int &copies_, &moves_, &destruction_;
    long state_ = 0;

    TracesBase() = default;
    TracesBase(int &c, int &m, int &d):
        copies_{c}, moves_{m}, destruction_{d}
    {}

    void operator()(int s) { state_ = s; }
};

struct Traces: TracesBase {
    Traces(const Traces &model): TracesBase{model} { ++copies_; }
    Traces(Traces &&source) noexcept: TracesBase{std::move(source)} { ++moves_; }
    Traces(int &c, int &m, int &d): TracesBase{c, m , d} {}
    ~Traces() { destruction_ = 1; }
};

static_assert(zoo::canUseValueSemantics<Traces>(BigSize, 8), "");
