#ifndef TEST_CALLABLE_HELPER
#define TEST_CALLABLE_HELPER

#include "any.h"

/// Uses Argument Dependent Lookup
struct BeforeAnyCallableEraserADL: LargeTypeEraser {};
inline void swap(BeforeAnyCallableEraserADL &e1, BeforeAnyCallableEraserADL &e2) noexcept {
    auto &upcasted = static_cast<LargeTypeEraser &>(e1);
    swap(upcasted, e2);
}

/// Uses the template with the policy as argument, works
struct BeforeAnyCallableEraserP: LargeTypeEraser {};
inline void swap(BeforeAnyCallableEraserP &e1, BeforeAnyCallableEraserP &e2) noexcept {
    zoo::swap<LargePolicy>(e1, e2);
}

/// Uses the template on the eraser as argument, won't use optimized overload
struct BeforeAnyCallableEraserT: LargeTypeEraser {};
inline void swap(BeforeAnyCallableEraserT &e1, BeforeAnyCallableEraserT &e2) noexcept {
    zoo::swap<LargeTypeEraser>(e1, e2);
}

struct AfterAnyCallableEraser: LargeTypeEraser {};

#include "zoo/function.h"

inline void swap(AfterAnyCallableEraser &e1, AfterAnyCallableEraser &e2) noexcept {
    zoo::swap<LargePolicy>(e1, e2);
}

struct TracesBase {
    int &copies_, &moves_, &destruction_;
    long state_ = 0;

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

#endif

