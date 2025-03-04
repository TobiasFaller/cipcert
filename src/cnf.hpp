#pragma once
#include <cstdint>
#include <ostream>
#include <vector>

struct CNF {
  int64_t n{}, m{};
  bool has_next{};
  CNF() = default;
  CNF(int64_t n, int64_t m, bool has_next) : n(n), m(m), has_next(has_next) {
    clauses.reserve(m);
  }
  std::vector<std::vector<int64_t>> clauses{};
};
std::ostream &operator<<(std::ostream &os, const CNF &cnf);
