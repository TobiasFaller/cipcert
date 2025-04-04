#pragma once
#include <vector>

#include "cnf.hpp"

struct Cip {
  Cip(const char *path);
  void try_parse_simulation(std::ifstream &file);

  CNF init, trans, target;
  int64_t size;
  std::vector<std::pair<int64_t, int64_t>> simulation;
};

std::ostream &operator<<(std::ostream &os, const Cip &dimspec);
