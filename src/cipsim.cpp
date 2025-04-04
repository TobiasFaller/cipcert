#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ranges>
#include <vector>

#include "cip.hpp"
#include "ciptrace.hpp"

#ifdef QUIET
#define MSG \
  if (0) std::cout
#else
#define MSG std::cout << "dimsim: "
#endif

auto param(int argc, char *argv[]) {
  if (argc > 1 && !strcmp(argv[1], "--version")) {
    std::cout << VERSION << '\n';
    exit(0);
  }
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <model.dimspec> <model.trace>\n";
    exit(1);
  }
  return std::tuple{argv[1], argv[2]};
}

bool check_clauses(const CNF &cnf, const std::vector<int64_t>& current, const std::vector<int64_t>& next = {}) {
  for (auto const &clause : cnf.clauses) {
    for (auto const &lit : clause)
      if (std::abs(lit) <= cnf.n) {
        if (std::binary_search(current.begin(), current.end(), lit))
          goto next_clause;
      } else {
        if (std::binary_search(next.begin(), next.end(), lit - ((lit < 0) ? -cnf.n : cnf.n)))
          goto next_clause;
      }
    return false;
  next_clause:
    continue;
  }
  return true;
}

int check_trace(const Cip& model, const Ciptrace& trace) {
  auto const &timeframes { trace.timeframes };
  if (timeframes.size() < 1)
    return 1; // No timeframes
  if (!check_clauses(model.init, timeframes[0]))
    return 2; // INIT UNSAT
  for (size_t i { 0u }; i < timeframes.size(); ++i) {
    if (check_clauses(model.target, timeframes[i]))
      return 0; // TARGET SAT
    if (i + 1 < timeframes.size()
        && !check_clauses(model.trans, timeframes[i], timeframes[i + 1]))
      return 3; // TRANS UNSAT
  }
  return 4; // No SAT found
}

int main(int argc, char **argv) {
  auto [model_path, trace_path] = param(argc, argv);
  MSG << "Checking Traces for Dimspec\n";
  MSG << VERSION " " GITID "\n";
  Cip model(model_path);
  Ciptrace trace(trace_path);
  return check_trace(model, trace);
}
