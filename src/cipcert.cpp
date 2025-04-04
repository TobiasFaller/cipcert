#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ranges>
#include <vector>

#include "cip.hpp"
#include "qcir.hpp"

#ifdef QUIET
#define MSG \
  if (0) std::cout
#else
#define MSG std::cout << "cipcert: "
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
    std::cerr << "Usage: " << argv[0] << " <model.cip> <witness.cip> [";
    for (const char *o : checks)
      std::cerr << " <" << o << ">";
    std::cerr << " ]\n";
    exit(1);
  }
  for (int i = 3; i < argc; ++i)
    checks[i - 3] = argv[i];
  return std::tuple{argv[1], argv[2], checks};
}


std::pair<std::vector<int64_t>, std::vector<int64_t>> index_consecutively(Cip &witness,
                                                                          Cip &model) {
  auto sign = [](int64_t l) { return l < 0 ? -1 : 1; };
  const int64_t witness_size { witness.size };
  const int64_t model_size { model.size };
  const int64_t new_size { witness.size + model.size };
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

  for (auto &cnf : {&witness.init, &witness.trans, &witness.target}) {
    cnf->n = new_size;
    for (auto &clause : cnf->clauses)
      for (auto &l : clause)
        l = witness_map[std::abs(l)] * sign(l);
  }

  for (auto &cnf : {&model.init, &model.trans, &model.target}) {
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

void reset(const char *path, const Cip &witness, const Cip &model,
           const std::vector<int64_t> &extension) {
  // check that forall(S) INIT => exist(S'\S) INIT'
  QCir check {
    qneg(                                        // !(INIT => INIT')
      qimply(                                    // INIT => INIT'
        to_qcir(model.init),                     // INIT
        to_qcir(witness.init)                    // INIT'
      )) };
  // expect exists(S) forall(S'\S): !(INIT => INIT') = UNSAT
  for (auto l : extension)
    check.vars[l - 1] = QVarType::ForAll;
  std::ofstream os { path };
  os << check;
}

void transition(const char *path, const Cip &witness, const Cip &model,
           const std::vector<int64_t> &next_extension) {
  // check that forall(S) TRANS => exist(S'\S) TRANS'
  QCir check {
    qneg(                                        // !(TRANS => TRANS')
      qimply(                                    // TRANS => TRANS'
        to_qcir(model.trans),                    // TRANS
        to_qcir(witness.trans)                   // TRANS'
      )) };
  // expect exists(S) forall(S'\S) !(TRANS => TRANS') = UNSAT
  for (auto l : next_extension)
    check.vars[l - 1] = QVarType::ForAll;
  std::ofstream os { path };
  os << check;
}

void property(const char *path, const Cip &witness, const Cip &model,
           const std::vector<int64_t> &extension) {
  // check that forall(S) -TARGET' => exist(S'\S) -TARGET
  QCir check {
    qneg(                                        // !(-TARGET' => -TARGET)
      qimply(                                    // -TARGET' => -TARGET
        qneg(to_qcir(witness.target)),           // -TARGET'
        qneg(to_qcir(model.target))              // -TARGET
      )) };
  // expect exists(S) forall(S'\S) !(-TARGET' => -TARGET) = UNSAT
  for (auto l : extension)
    check.vars[l - 1] = QVarType::ForAll;
  std::ofstream os { path };
  os << check;
}

void base(const char *path, const Cip &witness) {
  // check that forall(S') INIT' => -TARGET'
  QCir check {
    qneg(                                        // !(INIT' => -TARGET')
      qimply(                                    // INIT' => -TARGET'
          to_qcir(witness.init),                 // INIT'
          qneg(to_qcir(witness.target))          // -TARGET'
      )) };
  // expect exists(S') !(INIT' => -TARGET') = UNSAT
  std::ofstream os { path };
  os << check;
}

void step(const char *path, const Cip &witness) {
  // check that forall(S') -TARGET0' ^ TRANS' => -TARGET1'
  QCir check {
    qneg(                                        // !(-TARGET0' ^ TRANS' => -TARGET1')
      qimply(                                    // -TARGET0' ^ TRANS' => -TARGET1'
        qand(                                    // -TARGET0' ^ TRANS'
          qneg(to_qcir(witness.target)),         // -TARGET0'
          to_qcir(witness.trans)),               // TRANS'
        qneg(qnext(to_qcir(witness.target)))     // -TARGET1'
      )) };
  // expect exists(S') !(-TARGET0' ^ TRANS' => -TARGET1') = UNSAT
  std::ofstream os { path };
  os << check;
}

int main(int argc, char **argv) {
  auto [model_path, witness_path, checks] = param(argc, argv);
  MSG << "Certify Model Checking Witnesses in Cip\n";
  MSG << VERSION " " GITID "\n";
  Cip model(model_path), witness(witness_path);
  auto [extension, next_extension] { index_consecutively(witness, model) };
  reset(checks[0], witness, model, extension);
  transition(checks[1], witness, model, next_extension);
  property(checks[2], witness, model, extension);
  base(checks[3], witness);
  step(checks[4], witness);
}
