#pragma once

#include "STD.h"

class FastRandom {
 public:
  // Seed for the random number generator
  explicit FastRandom(uint32_t seed = 123456789) : seed(seed) {}

  // Generate a random number less than the bound
  int less_than(int bound) { return fast_random(0, bound - 1); }

  template <integral T>
  inline T random() {
    uniform_int_distribution<T> uniform_dist(numeric_limits<T>::min(),
                                             numeric_limits<T>::max());
    return uniform_dist(engine);
  }

 private:
  uint32_t seed;
  random_device r{};
  mt19937 engine{r()};

  // Fast random number generator xorshift32
  int fast_random(int min, int max) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    int range = max - min + 1;
    return min + static_cast<int>(seed % range);
  }
};

inline FastRandom gen;
