#pragma once

#include <chrono>

namespace opossum {

/**
 * Starts a std::chrono::high_resolution_clock base timer on construction and returns/reset measurement on calling lap()
 */
class Timer final {
 public:
  Timer();

  std::chrono::nanoseconds lap();

 private:
  std::chrono::high_resolution_clock::time_point _begin;
};

}  // namespace opossum