#include "dimspec.hpp"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
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
      "reset.cir", "transition.cir", "property.cir",
      "base.cir",  "step.cir",
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
  std::vector<std::pair<int64_t, int64_t>> shared;
  int64_t new_size { 0 };
  int64_t model_size { model.size() };
  int64_t model_shift { 0 };
  int64_t model_next_shift { 0 };
  int64_t witness_size { witness.size() };
  int64_t witness_shift { 0 };
  int64_t witness_next_shift { 0 };
  if (witness.simulation.empty()) {
    MSG << "No witness mapping found, using default\n";
    assert (witness_size >= model_size);
    // Remap variables [model|mnext] and [witness|wnext] to [witness|wnext].
    // Note: The number of variables in the model might be less than the witnesse's.
    // => We might need to shift the model's next variables to behind witness.
    new_size = witness_size; // doesn't include next variables
    model_shift = 0;
    model_next_shift = (witness_size - model_size);
    witness_shift = 0;
    witness_next_shift = 0;
    shared.reserve(2 * model_size);
    for (int64_t m { 1 }; m < model_size + 1; ++m) {
      shared.push_back({ m , m });
      shared.push_back({ m + model_size, m + model_size + model_next_shift });
    }
  } else {
    // Remap variables [model|mnext] and [witness|wnext] to [witness|model|wnext|mnext].
    // Note: The shared variables are mapped according to the user-provided data.
    new_size = model_size + witness_size; // doesn't include next variables
    model_shift = witness_size;
    model_next_shift = 2 * witness_size;
    witness_shift = 0;
    witness_next_shift = model_size;
    shared.reserve(2 * witness.simulation.size());
    for (auto [w, m] : witness.simulation) {
      shared.push_back({ m, w });
      shared.push_back({ m + model_size, w + witness_size + witness_next_shift });
    }
  }
  // The size of the model might be smaller than the witness.
  // If we are re-mapping with a user-provided mapping then the wittness size changes too.
  // => We need to re-map the model current and next variables.
  if (model_size != new_size) {
    std::vector<int64_t> model_map (2 * model_size);
    std::iota(std::begin(model_map), std::begin(model_map) + model_size, 1 + model_shift);
    std::iota(std::begin(model_map) + model_size, std::end(model_map),   1 + model_size + model_next_shift);
    for (auto& [m, w] : shared)
      model_map[m - 1] = w;
    for (auto &cnf : {&model.initial, &model.universal, &model.goal, &model.transition}) {
      cnf->n = new_size;
      for (auto &clause : cnf->clauses)
        for (auto &l : clause) {
          l = model_map[std::abs(l) - 1] * ((l < 0) ? -1 : 1);
        }
    }
    model.transition.n = 2 * new_size;
  }
  // If we are re-mapping with a user-provided mapping then the wittness size changes.
  // => We need to re-map the witness next variables.
  if (witness_size != new_size) {
    for (auto &cnf : {&witness.initial, &witness.universal, &witness.goal, &witness.transition}) {
      cnf->n = new_size;
      for (auto &clause : cnf->clauses)
        for (auto &l : clause)
          if (std::abs(l) <= witness_size) {
            l += witness_shift * ((l < 0) ? -1 : 1);
          } else {
            l += witness_next_shift * ((l < 0) ? -1 : 1);
          }
    }
    witness.transition.n = 2 * new_size;
  }
  return shared;
}

void reset(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<std::pair<int64_t, int64_t>> &shared) {
  // check that forall(s) i ^ u => exist(s'\s) i' ^ u'
  QCir check {
    qneg(                                        // !(i ^ u => i' ^ u')
      qimply(                                    // i ^ u => i' ^ u'
        qand(                                    // i ^ u
          to_qcir(model.initial),                // i
          to_qcir(model.universal)),             // u
        qand(                                    // i' ^ u'
          to_qcir(witness.initial),              // i'
          to_qcir(witness.universal))            // u'
      ))};
  // expect exists(s) forall(s'\s): !(i ^ u => i' ^ u') = UNSAT
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::ForAll);
  for (auto& [model, witness] : shared)
    check.vars[witness - 1] = QVarType::Exists;
  std::ofstream os { path };
  os << check;
}

void transition(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<std::pair<int64_t, int64_t>> &shared) {
  // check that forall(s) t ^ u0 ^ u1 ^ u0' => exist(s'\s) t' ^ u1'
  QCir check {
    qneg(                                        // !(t ^ u0 ^ u1 ^ u0' => t' ^ u1')
      qimply(                                    // t ^ u0 ^ u1 ^ u0' => t' ^ u1'
        qand(                                    // t ^ u0 ^ u1 ^ u0'
          qand(                                  // t ^ u0
            to_qcir(model.transition),           // t
            to_qcir(model.universal)),           // u0
          qand(                                  // u1 ^ u0'
            qnext(to_qcir(model.universal)),     // u1
            to_qcir(witness.universal))),        // u0'
        qand(                                    // t' ^ u1'
          to_qcir(witness.transition),           // t'
          qnext(to_qcir(witness.universal)))     // u1'
      ))};
  // expect exists(s) forall(s'\s) !(t ^ u0 ^ u1 ^ u0' => t' ^ u1') = UNSAT
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::ForAll);
  for (auto& [model, witness] : shared)
    check.vars[witness - 1] = QVarType::Exists;
  std::ofstream os { path };
  os << check;
}

void property(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<std::pair<int64_t, int64_t>> &shared) {
  // check that forall(s) u ^ u' => exist(s'\s) (-g => -g')
  QCir check {
    qneg(                                        // !(u ^ u' => (-g => -g'))
      qimply(                                    // u ^ u' => (-g => -g')
        qand(                                    // u ^ u'
          to_qcir(model.universal),              // u
          to_qcir(witness.universal)),           // u'
        qimply(                                  // -g => -g'
          qneg(to_qcir(model.goal)),             // -g
          qneg(to_qcir(witness.goal)))           // -g'
      ))};
  // expect exists(s) forall(s'\s) !(u ^ u' => (-g => -g')) = UNSAT
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::ForAll);
  for (auto& [model, witness] : shared)
    check.vars[witness - 1] = QVarType::Exists;
  std::ofstream os { path };
  os << check;
}

void base(const char *path, const Dimspec &witness) {
  // check that forall(s') i' ^ u' => -g'
  QCir check {
    qneg(                                        // !(i' ^ u' => -g')
      qimply(                                    // i' ^ u' => -g'
        qand(                                    // i' ^ u'
          to_qcir(witness.initial),              // i'
          to_qcir(witness.universal)),           // u'
        qneg(to_qcir(witness.goal))              // -g'
      ))};
  // expect exists(s') !(i' ^ u' => -g') = UNSAT
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::Exists);
  std::ofstream os { path };
  os << check;
}

void step(const char *path, const Dimspec &witness) {
  // check that forall(s') -g0' ^ t' ^ u0' ^ u1' => -g1'
  QCir check {
    qneg(                                        // !(-g0' ^ t' ^ u0' ^ u1' => -g1')
      qimply(                                    // -g0' ^ t' ^ u0' ^ u1' => -g1'
        qand(                                    // -g0' ^ t' ^ u0' ^ u1'
          qand(                                  // -g0' ^ t'
            qneg(to_qcir(witness.goal)),         // -g0'
            to_qcir(witness.transition)),        // t'
          qand(                                  // t' ^ u0'
            to_qcir(witness.universal),          // u0'
            qnext(to_qcir(witness.universal)))), // u1'
        qneg(qnext(to_qcir(witness.goal)))       // -g1'
      ))};
  // expect exists(s') !(-g0 ^ t' ^ u0' => -g1') = UNSAT
  std::fill(std::begin(check.vars), std::end(check.vars), QVarType::Exists);
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
