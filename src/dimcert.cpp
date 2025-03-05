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


std::pair<std::vector<int64_t>, std::vector<int64_t>> index_consecutively(Dimspec &witness,
                                                                          Dimspec &model) {
  auto sign = [](int64_t l) { return l < 0 ? -1 : 1; };
  const int64_t witness_size { witness.size() };
  const int64_t model_size { model.size() };
  const int64_t new_size { witness.size() + model.size() };
  const int64_t max_shared_size { std::min(witness_size, model_size) };

  const int64_t witness_shift { 0 };
  const int64_t witness_next_shift { model_size };
  const int64_t model_shift { witness_size };
  const int64_t model_next_shift { 2 * witness_size };

  std::vector<std::pair<int64_t, int64_t>> shared { witness.simulation };

  if (shared.empty()) {
    MSG << "No witness mapping found, using default\n";
    shared.reserve(max_shared_size);
    for (int64_t l { 1 }; l < max_shared_size + 1; ++l) {
      shared.push_back({ l , l });
    }
  }

  std::vector<int64_t> witness_map(2 * witness_size + 1);
  for (int64_t i { 1 }; i < witness_map.size(); ++i)
    witness_map[i] = i + (i - 1 < witness_size ? witness_shift : witness_next_shift);
  std::vector<int64_t> model_map(2 * model_size + 1);
  for (size_t i { 1 }; i < model_map.size(); ++i)
    model_map[i] = i + (i - 1 < model_size ? model_shift : model_next_shift);

  for(auto [w, m] : shared) {
    model_map[m] = witness_map[w];
    model_map[m + model_size] = witness_map[w + witness_size];
  }

  for (auto &cnf : {&witness.initial, &witness.universal, &witness.goal, &witness.transition}) {
    cnf->n = new_size;
    for (auto &clause : cnf->clauses)
      for (auto &l : clause)
        l = witness_map[std::abs(l)] * sign(l);
  }

  for (auto &cnf : {&model.initial, &model.universal, &model.goal, &model.transition}) {
    cnf->n = new_size;
    for (auto &clause : cnf->clauses)
      for (auto &l : clause)
        l = model_map[std::abs(l)] * sign(l);
  }

  std::vector<int64_t> extension, next_extension;
  std::vector<bool> is_shared(witness_size + 1);
  for (auto [w, _] : shared)
    is_shared[w] = true;
  for (int i = 1; i < witness_size + 1; ++i){
    if (is_shared[i])
      continue;
    extension.push_back(witness_map[i]);
    next_extension.push_back(witness_map[i + witness_size]);
  }

  return { extension, next_extension };
}

void reset(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<int64_t> &extension) {
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
      )) };
  // expect exists(s) forall(s'\s): !(i ^ u => i' ^ u') = UNSAT
  for (auto l : extension)
    check.vars[l - 1] = QVarType::ForAll;
  std::ofstream os { path };
  os << check;
}

void transition(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<int64_t> &next_extension) {
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
      )) };
  // expect exists(s) forall(s'\s) !(t ^ u0 ^ u1 ^ u0' => t' ^ u1') = UNSAT
  for (auto l : next_extension)
    check.vars[l - 1] = QVarType::ForAll;
  std::ofstream os { path };
  os << check;
}

void property(const char *path, const Dimspec &witness, const Dimspec &model,
           const std::vector<int64_t> &extension) {
  // check that forall(s) u ^ u' => exist(s'\s) (-g => -g')
  QCir check {
    qneg(                                        // !(u ^ u' => (g => g'))
      qimply(                                    // u ^ u' => (g => g')
        qand(                                    // u ^ u'
          to_qcir(model.universal),              // u
          to_qcir(witness.universal)),           // u'
        qimply(                                  // g => g'
          to_qcir(model.goal),                   // g
          to_qcir(witness.goal))                 // g'
      )) };
  // expect exists(s) forall(s'\s) !(u ^ u' => (g => g')) = UNSAT
  for (auto l : extension)
    check.vars[l - 1] = QVarType::ForAll;
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
      )) };
  // expect exists(s') !(i' ^ u' => -g') = UNSAT
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
      )) };
  // expect exists(s') !(-g0 ^ t' ^ u0' ^ u1' => -g1') = UNSAT
  std::ofstream os { path };
  os << check;
}

int main(int argc, char **argv) {
  auto [model_path, witness_path, checks] = param(argc, argv);
  MSG << "Certify Model Checking Witnesses in Dimspec\n";
  MSG << VERSION " " GITID "\n";
  Dimspec model(model_path), witness(witness_path);
  auto [extension, next_extension] { index_consecutively(witness, model) };
  reset(checks[0], witness, model, extension);
  transition(checks[1], witness, model, next_extension);
  property(checks[2], witness, model, extension);
  base(checks[3], witness);
  step(checks[4], witness);
}
