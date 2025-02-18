#pragma once
#include <vector>

#include "cnf.hpp"

struct Dimspec {
  Dimspec(const char *path);
  void try_parse_simulation(std::ifstream &file);
  int64_t size() const;

  CNF initial, universal, goal, transition;
  std::vector<std::pair<int64_t, int64_t>> simulation;
};

std::ostream &operator<<(std::ostream &os, const Dimspec &dimspec);
