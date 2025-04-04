#pragma once
#include <cstdint>
#include <ostream>
#include <vector>

struct CNF {
  int64_t n{}, m{};
  CNF() = default;
  CNF(int64_t n, int64_t m): n(n), m(m) { clauses.reserve(m); }
  std::vector<std::vector<int64_t>> clauses{};
};
std::ostream &operator<<(std::ostream &os, const CNF &cnf);
