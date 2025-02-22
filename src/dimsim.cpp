#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ranges>
#include <vector>

#include "dimspec.hpp"
#include "dimtrace.hpp"

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

bool check_clause(const std::vector<int64_t> &clause, const std::vector<int64_t>& values) {
  for (auto const &lit : clause)
    if (std::binary_search(values.begin(), values.end(), lit))
      return true;
  return false;
}

bool check_clauses(const CNF &cnf, const std::vector<int64_t>& values) {
  for (auto const &clause : cnf.clauses)
    if (!check_clause(clause, values))
      return false;
  return true;
}

int check_trace(const Dimspec& model, const Dimtrace& trace) {
  if (trace.timeframes.size() < 1) { return 1; }
  if (!check_clauses(model.initial, trace.timeframes[0])) return 2;
  for (size_t index { 0u }; index < trace.timeframes.size(); ++index) {
    if (!check_clauses(model.universal, trace.timeframes[index]))
      if (index == 0u) return 3; else continue;
    if (check_clauses(model.goal, trace.timeframes[index])) return 0;
  }
  return 4;
}

int main(int argc, char **argv) {
  auto [model_path, trace_path] = param(argc, argv);
  MSG << "Checking Traces for Dimspec\n";
  MSG << VERSION " " GITID "\n";
  Dimspec model(model_path);
  Dimtrace trace(trace_path);
  return check_trace(model, trace);
}
