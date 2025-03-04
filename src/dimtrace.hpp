#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

struct Dimtrace {
  Dimtrace(char *path);
  std::vector<std::vector<int64_t>> timeframes;
};
std::ostream &operator<<(std::ostream &os, const Dimtrace &trace);
