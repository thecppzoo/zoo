#ifndef OPERATIONS_H
#define OPERATIONS_H

/// \file Operators.h

namespace zoo {

struct Multiplication {};
struct Substraction { template<typename A> static inline A execute(A x, A y); };
struct Exponentiation {};
struct Division { template<typename A> static inline A execute(A x, A y); };

template<typename Operation, typename Argument>
Argument egyptian(Argument x, Argument y, Operation);

/// Declares the traits template, needs to be specialized
template<typename Operator> struct OperatorTraits;

template<> struct OperatorTraits<Multiplication> {
    template<typename A>
    static inline A operate(A x, A y);

    using Inverse = Substraction;
};

template<> struct OperatorTraits<Exponentiation> {
    template<typename A>
    static A inline operate(A x, A y);

    using Inverse = Division;
};


template<typename T>
inline bool isOdd(T v) { return v % 2; }

template<typename T>
inline T half(T v) { return v / 2; }

}

#endif /* OPERATIONS_H */


