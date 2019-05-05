#ifndef OPERATIONS_IMPL_H
#define OPERATIONS_IMPL_H

#include "Operations.h"

namespace zoo {

template<typename A>
A Substraction::execute(A x, A y) { return x - y; };

template<typename A>
A Division::execute(A x, A y) { return x/y; }

/// \file OperatorTraits_impl.h
template<typename A>
A OperatorTraits<Multiplication>::operate(A x, A y) {
    return x + y;
}

template<typename A>
A OperatorTraits<Exponentiation>::operate(A x, A y) {
    return egyptian(x, y, Multiplication{});
}

template<typename Operation, typename Argument>
Argument egyptian(Argument x, Argument y, Operation) {
    if(0 == y) { return 0; }
    while(!isOdd(y)) {
        x = OperatorTraits<Operation>::operate(x, x);
        y = half(y);
    }
    auto result = x;
    for(;;) {
        y = half(y);
        x = OperatorTraits<Operation>::operate(x, x);
        if(isOdd(y)) {
            result = OperatorTraits<Operation>::operate(result, x);
        } else { if(0 == y) { return result; } }
    }
}

}

#endif /* OPERATIONS_IMPL_H */


