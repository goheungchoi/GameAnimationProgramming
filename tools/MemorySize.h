#pragma once

#include <cmath>
#include <iostream>
#include <string>

class MemorySize {
 public:
  constexpr MemorySize(unsigned long long bytes) : bytes_(bytes) {}

  constexpr unsigned long long bytes() const { return bytes_; }
  constexpr double kib() const { return static_cast<double>(bytes_) / 1024; }
  constexpr double mib() const {
    return static_cast<double>(bytes_) / (1024 * 1024);
  }
  constexpr double gib() const {
    return static_cast<double>(bytes_) / (1024 * 1024 * 1024);
  }

  std::string to_string() const {
    constexpr unsigned long long GiB = 1024ull * 1024 * 1024;
    constexpr unsigned long long MiB = 1024ull * 1024;
    constexpr unsigned long long KiB = 1024ull;

    char buffer[64];
    if (bytes_ >= GiB) {
      std::snprintf(buffer, sizeof(buffer), "%.2f GiB", gib());
    } else if (bytes_ >= MiB) {
      std::snprintf(buffer, sizeof(buffer), "%.2f MiB", mib());
    } else if (bytes_ >= KiB) {
      std::snprintf(buffer, sizeof(buffer), "%.2f KiB", kib());
    } else {
      std::snprintf(buffer, sizeof(buffer), "%llu Bytes", bytes_);
    }
    return buffer;
  }

 private:
  unsigned long long bytes_;
};

// User-defined literals
constexpr MemorySize operator""_B(unsigned long long val) {
  return MemorySize(val);
}
constexpr MemorySize operator""_KiB(unsigned long long val) {
  return MemorySize(val * 1024);
}
constexpr MemorySize operator""_MiB(unsigned long long val) {
  return MemorySize(val * 1024 * 1024);
}
constexpr MemorySize operator""_GiB(unsigned long long val) {
  return MemorySize(val * 1024 * 1024 * 1024);
}
