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
    Argument
        delta = x,
        result = OperatorTraits<Operation>::initializer;
    while(0 != y) { // the argument must be comparable to 0
        if(isOdd(y)) {
            auto newResult = OperatorTraits<Operation>::operate(result, delta);
            result = newResult;
        }
        delta = OperatorTraits<Operation>::operate(delta, delta);
        y = half(y);
    }
    return result;
}

}

#endif /* OPERATIONS_IMPL_H */


