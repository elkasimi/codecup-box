#pragma once

#if !defined(__APPLE__)
#include <immintrin.h>

#include <ext/pool_allocator.h>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using std::array;
using std::bitset;
using std::cerr;
using std::cin;
using std::convertible_to;
using std::countr_zero;
using std::cout;
using std::discrete_distribution;
using std::endl;
using std::fixed;
using std::function;
using std::greater;
using std::holds_alternative;
using std::integral;
using std::is_same_v;
using std::less;
using std::make_unique;
using std::map;
using std::max;
using std::min;
using std::mt19937;
using std::numeric_limits;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::popcount;
using std::pow;
using std::random_device;
using std::setprecision;
using std::size_t;
using std::sqrt;
using std::streambuf;
using std::string;
using std::swap;
using std::to_string;
using std::tuple;
using std::uniform_int_distribution;
using std::unique_ptr;
using std::variant;

template <class Key, class Compare = std::less<Key>,
          class Allocator = std::allocator<Key>>
using TreeSet = std::set<Key, Compare, Allocator>;

#if !defined(__APPLE__)
template <class T>
using CustomAllocator = __gnu_cxx::__pool_alloc<T>;
#else
template <class T>
using CustomAllocator = std::allocator<T>;
#endif

template <class Key, class T, class Hash, class KeyEqual>
using HashMap = std::unordered_map<Key, T, Hash, KeyEqual,
                                   CustomAllocator<pair<const Key, T>>>;

template <class T>
using vector = std::vector<T, CustomAllocator<T>>;

namespace ranges = std::ranges;

// A stream buffer that discards any output
class null_streambuf : public streambuf {
 protected:
  // Override the overflow method to discard the output
  int overflow(int c) override {
    return c;  // Discard character by returning it as if it was written
  }
};

// A null output stream
class null_ostream : public ostream {
 public:
  null_ostream() : ostream(&null_buf) {}

 private:
  null_streambuf null_buf;  // Use null_streambuf to discard output
};

inline null_ostream NULL_OUT;

class iota_iterator {
  int value;

 public:
  explicit iota_iterator(int start) : value(start) {}

  int operator*() const { return value; }

  iota_iterator& operator++() {
    ++value;
    return *this;
  }

  bool operator!=(const iota_iterator& other) const {
    return value != other.value;
  }
};

class iota_view {
  int start_;
  int end_;

 public:
  iota_view(int start, int end) : start_(start), end_(end) {}

  iota_iterator begin() const { return iota_iterator(start_); }
  iota_iterator end() const { return iota_iterator(end_); }
};

template <class Array>
void array_log(const char* name, const Array& arr) {
  cerr << name << " = ";
  for (auto& ai : arr) cerr << ai << " ";
  cerr << endl;
}
