#include "dimspec.hpp"

#include <cstring>
#include <iostream>
#include <fstream>
#include <ranges>
#include <vector>

#include "dimspec.hpp"
#include "qcir.hpp"

#ifdef QUIET
#define MSG \
  if (0) std::cout
#else
#define MSG std::cout << "dimcert: "
#endif

auto param(int argc, char *argv[]) {
  std::vector<const char *> checks{
      "reset.dimspec", "transition.dimspec", "property.dimspec",
      "base.dimspec",  "step.dimspec",
  };
  if (argc > 1 && !strcmp(argv[1], "--version")) {
    std::cout << VERSION << '\n';
    exit(0);
  }
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <model.dimspec> <witness.dimspec> [";
    for (const char *o : checks)
      std::cerr << " <" << o << ">";
    std::cerr << " ]\n";
    exit(1);
  }
  for (int i = 3; i < argc; ++i)
    checks[i - 3] = argv[i];
  return std::tuple{argv[1], argv[2], checks};
}

std::vector<std::pair<int64_t, int64_t>> index_consecutively(Dimspec &witness,
                                                             Dimspec &model) {
  const int64_t offset = witness.simulation.size() ? witness.size() : 0;
  std::vector<int64_t> model_map;
  model_map.reserve(model.size());
  for (int64_t i = 0; i < model.size(); ++i)
    model_map.push_back(i + offset);
  for (auto [w, m] : witness.simulation)
    model_map[m] = w;
  for (auto &f :
       {&model.initial, &model.universal, &model.goal, &model.transition})
    for (auto &c : f->clauses)
      for (auto &l : c)
        l = model_map[l];
  return witness.simulation;
}

void reset(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<std::pair<int64_t, int64_t>> &shared) {
  QCir check {
    qneg(                                     // !(i ^ u => i' ^ u')
      qimply(                                 // i ^ u => i' ^ u'
        qand(                                 // i ^ u
          to_qcir(model.initial),             // i
          to_qcir(model.universal)),          // u
        qand(                                 // i' ^ u'
          to_qcir(witness.initial),           // i'
          to_qcir(witness.universal))         // u'
      ))};
  // exists(s) forall(s'\\s): !(i ^ u => i' ^ u')
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::ForAll);
  for (auto& [model, witness] : shared)
    check.vars[witness - 1] = QVarType::Exists;
  std::ofstream os { path };
  os << check;
}

void transition(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<std::pair<int64_t, int64_t>> &shared) {
  QCir check {
    qneg(                                     // !(t ^ u0 ^ u1 ^ u' => t' ^ u1')
      qimply(                                 // t ^ u0 ^ u1 ^ u' => t' ^ u1'
        qand(                                 // t ^ u0 ^ u1 ^ u'
          qand(                               // t ^ u0
            to_qcir(model.transition),        // t
            to_qcir(model.universal)),        // u0
          qand(                               // u1 ^ u'
            qprime(to_qcir(model.universal)), // u1
            to_qcir(witness.universal))),     // u'
        qand(                                 // t' ^ u1'
          to_qcir(witness.transition),        // t'
          qprime(to_qcir(witness.universal))) // u1'
      ))};
  // exists(s) forall(s'\\s): !(t ^ u0 ^ u1 ^ u' => t' ^ u1')
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::ForAll);
  for (auto& [model, witness] : shared)
    check.vars[witness - 1] = QVarType::Exists;
  std::ofstream os { path };
  os << check;
}

void property(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<std::pair<int64_t, int64_t>> &shared) {
  QCir check {
    qneg(                                     // !(u ^ u' => (-g => -g'))
      qimply(                                 // u ^ u' => (-g => -g')
        qand(                                 // u ^ u'
          to_qcir(model.universal),           // u
          to_qcir(witness.universal)),        // u'
        qimply(                               // -g => -g'
          qneg(to_qcir(model.goal)),          // -g
          qneg(to_qcir(witness.goal)))        // -g'
      ))};
  // exists(s) forall(s'\\s): !(u ^ u' => (-g => -g'))
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::ForAll);
  for (auto& [model, witness] : shared)
    check.vars[witness - 1] = QVarType::Exists;
  std::ofstream os { path };
  os << check;
}

void base(const char *path, const Dimspec &witness) {
  QCir check {
    qneg(                                     // !(i' ^ u' => -g')
      qimply(                                 // i' ^ u' => -g'
        qand(                                 // i' ^ u'
          to_qcir(witness.initial),           // i'
          to_qcir(witness.universal)),        // u'
        qneg(to_qcir(witness.goal))           // -g'
      ))};
  // exists(s'): !(i' ^ u' => -g')
  std::ofstream os { path };
  os << check;
}

void step(const char *path, const Dimspec &witness) {
  QCir check {
    qneg(                                     // !(-g0 ^ t' ^ u0' => -g1')
      qimply(                                 // -g0 ^ t' ^ u0' => -g1'
        qand(                                 // -g0 ^ t' ^ u0'
          qneg(to_qcir(witness.goal)),        // -g0
          qand(                               // t' ^ u0'
            to_qcir(witness.transition),      // t'
            to_qcir(witness.universal))),     // u0'
        qneg(qprime(to_qcir(witness.goal)))   // -g1'
      ))};
  // exists(s'): !(-g0 ^ t' ^ u0' => -g1')
  std::ofstream os { path };
  os << check;
}

int main(int argc, char **argv) {
  auto [model_path, witness_path, checks] = param(argc, argv);
  MSG << "Certify Model Checking Witnesses in Dimspec\n";
  MSG << VERSION " " GITID "\n";
  Dimspec model(model_path), witness(witness_path);
  auto shared { index_consecutively(witness, model) };
  reset(checks[0], witness, model, shared);
  transition(checks[1], witness, model, shared);
  property(checks[2], witness, model, shared);
  base(checks[3], witness);
  step(checks[4], witness);
}
