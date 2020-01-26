#ifndef ZOOTEST_FUNDAMENTALOPERATIONTRACING_H
#define ZOOTEST_FUNDAMENTALOPERATIONTRACING_H

#include <stdexcept>

namespace zoo {

struct TracesDestruction {
    int &trace_;

    TracesDestruction(int &trace): trace_(trace) {}
    ~TracesDestruction() { trace_ = 1; }
};

struct LargeTracesDestruction: TracesDestruction {
    using TracesDestruction::TracesDestruction;
    double whatever[3];
};

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

}

#endif //ZOOTEST_FUNDAMENTALOPERATIONTRACING_H
