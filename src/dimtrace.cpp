#include "dimtrace.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

Dimtrace::Dimtrace(char *path) {
  std::string line;
  std::ifstream stream(path);
  while (std::getline(stream, line)) {
    if (line.rfind("v", 0u) != 0u) { continue; }
    std::stringstream sstream(line.substr(1));
    int64_t value;
    sstream >> value;
    assert(value == timeframes.size());
    std::vector<int64_t> timeframe;
    while (sstream >> value && value)
      timeframe.push_back(value);
    std::sort(std::begin(timeframe), std::end(timeframe));
    timeframes.push_back(timeframe);
  }
}

std::ostream &operator<<(std::ostream &os, const Dimtrace &trace) {
  for (size_t index{0u}; index < trace.timeframes.size(); ++index) {
    os << "v" << index;
    for (auto &value : trace.timeframes[index])
      os << " " << value;
    os << " 0\n";
  }
  return os;
}
