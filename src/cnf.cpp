#include "cnf.hpp"

#include <cassert>

std::ostream &operator<<(std::ostream &os, const CNF &cnf) {
  os << "cnf " << cnf.n << " " << cnf.m << "\n";
  for (const auto &clause : cnf.clauses) {
    for (auto l : clause)
      os << l << " ";
    os << "0\n";
  }
  return os;
}
