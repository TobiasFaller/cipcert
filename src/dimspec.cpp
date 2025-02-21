#include "dimspec.hpp"
#include <cassert>

#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>

#include "cnf.hpp"

void Dimspec::try_parse_simulation(std::ifstream &file) {
  std::string s;
  std::getline(file, s);
  static const std::regex pattern(R"(\s*(-?\d+)\s*=\s*(-?\d+)\s*)");
  std::smatch matches;
  if (!std::regex_match(s, matches, pattern)) return;
  int64_t witness = std::stoi(matches[1].str());
  int64_t model = std::stoi(matches[2].str());
  simulation.push_back({witness, model});
}

void add_clause(const std::string &first, std::ifstream &file, CNF *cnf) {
  assert(cnf);
  int64_t l = std::stoi(first);
  if (!l) {
    cnf->clauses.push_back({});
    return;
  }
  assert(std::abs(l) <= (cnf->has_next ? cnf->n * 2 : cnf->n));
  cnf->clauses.push_back({l});
  while (file >> l && l) {
    assert(std::abs(l) <= (cnf->has_next ? cnf->n * 2 : cnf->n));
    cnf->clauses.back().push_back(l);
  }
}

CNF new_section(std::ifstream &file, bool has_next = false) {
  std::string s;
  file >> s;
  assert(s == "cnf");
  int64_t n, m;
  file >> n >> m;
  return CNF(has_next ? (n / 2) : n, m, has_next);
}

Dimspec::Dimspec(const char *path) {
  std::ifstream file(path);
  std::string s;
  CNF *current;
  while (file >> s)
    switch (s[0]) {
    case 'c': try_parse_simulation(file); break;
    case 'i': current = &(initial = new_section(file)); break;
    case 'u': current = &(universal = new_section(file)); break;
    case 'g': current = &(goal = new_section(file)); break;
    case 't': current = &(transition = new_section(file, true)); break;
    default: add_clause(s, file, current); break;
    }
}

int64_t Dimspec::size() const {
  const int64_t n = initial.n;
  assert(n == universal.n);
  assert(n == goal.n);
  assert(n == transition.n);
  return n;
}

std::ostream &operator<<(std::ostream &os, const Dimspec &dimspec) {
  os << "i " << dimspec.initial;
  os << "u " << dimspec.universal;
  os << "g " << dimspec.goal;
  os << "t " << dimspec.transition;
  return os;
}
