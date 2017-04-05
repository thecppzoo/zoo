# Pokerbotic

**Pokerbotic** is a poker engine.  It is being developed by a professional software engineer and a semi-professional poker player with professional knowledge of stochastic processes.

Currently, we have the hand evaluator framework, that achieves in normally available machines a rate of 100 million evaluations per second, that is, it classifies more than 100 million poker hands into what "four of a kind", etc. they are.

**The code today assumes the AMD64 architecture**, and support of the [BMI2 instructions](https://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets#BMI2_.28Bit_Manipulation_Instruction_Set_2.29).  AMD64/Intel is not essential to this code, just that the necessary adaptations have not been made.  You are welcome to help with this.

Currently, the code is a header-only framework with some use cases programmed in C++ 14.

## How to build it

### Prerequisites:

1. GCC compatible compiler.  We recommend Clang 3.9 or 4.0 specifically.  Benchmarks indicate Clang gives noticeably faster code.  The code uses GCC extensions in the way of builtins.
2. C++ 14.  In GCC or Clang, do not forget the option `-std=c++14`
3. Support for BMI2 instructions, activated with `-march=native` (preferred way) or specifically with `-mbmi2`
4. Test cases require the ["Catch" testing framework](https://github.com/philsquared/Catch).
5. Currently the code does not require a Unix/POSIX operating system (this code should be compilable in Windows64 through either gcc or clang), however, **we reserve the option to make the code incompatible with any operating system**.

There are several test programs available:

#### Unit tests at [src/main.cpp](https://github.com/thecppzoo/pokerbotic/blob/master/src/main.cpp)

Several unit tests.  This program illustrates how to use the engine framework.  To build it, at the project root, you may do this:

`clang++ -std=c++14 -Iinc -DTESTS -O3 -march=native -I../Catch/include src/main.cpp -o main`

Notice you have to define TESTS and indicate the path to the "Catch" testing framework.

#### [src/benchmarks.cpp](https://github.com/thecppzoo/pokerbotic/blob/master/src/benchmarks.cpp)

A program that measures the execution speed of several internal mechanisms.  To build it, at the project root, you may do this:

`clang++ -std=c++14 -Iinc -DBENCHMARKS -O3 -march=native src/benchmarks.cpp -o benchmarks`

This program can be run without arguments. It will generate all 7-card hands and time the execution of all evaluations.

#### [src/comparisonBenchmark.cpp](https://github.com/thecppzoo/pokerbotic/blob/master/src/comparisonBenchmark.cpp)

This program generates as in Texas Hold'em Poker, all possible 5-community cards, and proceeds to iterate over all two-player 2-card "pocket cards".

Because of the size of this search space, this program emits a current tally of execution every 100 million cases.

To build, for example:

`clang++ -std=c++14 -Iinc -DHAND_COMPARISON -O3 -march=native -o cb src/comparisonBenchmark.cpp`

Can be run without arguments.

## Next feature to be implemented

Currently, multithreaded partitioning of evaluations is being implemented.

## Documentation/User manual

Not yet written.  Most of the code available under the folder "ep/" is fully operational.

