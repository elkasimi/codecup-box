#pragma once

#include "STD.h"

inline auto get_time_point() { return std::chrono::system_clock::now(); }

inline auto get_delta_time_since(const auto& start) {
  using std::chrono::duration;
  using std::chrono::duration_cast;

  auto curr = get_time_point();
  return duration_cast<duration<double>>(curr - start).count();
}
