#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include "catch2/catch.hpp"

bool odd(int n) {
  return n & 1;
}

int half(int n) {
  return n >> 1;
}

int mult_acc(int r, int n, int a) {
  while (true) {
    if (odd(n)) {
      r = r + a;
      if (n == 1) return r;
    }
    n = half(n);
    a = a + a;
  }
}

int multiply(int n, int a) {
  while (!odd(n)) {
    a = a + a;
    n = half(n);
  }
  if (n == 1) return a;
  return mult_acc(a, half(n - 1), a + a);
}

int multiply15(int a) {
  int a2 = a + a;
  int a3 = a2 + a;
  int a6 = a3 + a3;
  int a12 = a6 + a6;
  return a12 + a3;
}

TEST_CASE("Multiply") {
    int a = 15, b = 234;
    BENCHMARK("Multiply by 15 using \"Multiply\"") {
        return multiply(b, multiply(b, multiply(b, a)));
    };
    BENCHMARK("Multiply by 15 using \"Multiply15\"") {
        return multiply15(multiply15(multiply15(b)));
    };
    BENCHMARK(
        "Multiply by 15 using \"Multiply\", 15 is a compile time constant"
    ) {
        return multiply(15, multiply(15, multiply(15, a)));
    };
}
