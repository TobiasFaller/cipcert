#include "qcir.hpp"

#include <numeric>

static QRef shift_ref(const QRef& ref, ssize_t shift);
static QRef merge_cir(QCir& left, const QCir& right);
static std::ostream &operator<<(std::ostream &os, const QVarType &var_type);
static std::ostream &operator<<(std::ostream &os, const QGateType &gate_type);
static std::ostream &operator<<(std::ostream &os, const QRefType &ref_type);
static std::ostream &operator<<(std::ostream &os, const QRef &ref);
static std::ostream &operator<<(std::ostream &os, const QGate &gate);

QCir to_qcir(const CNF& cnf) {
  return QCir(cnf);
}

QCir::QCir(const CNF& cnf):
    vars(cnf.n, QVarType::Exists),
    gates(cnf.m + 1, QGate { { }, QGateType::Or }),
    output(0, QRefType::Gate) {
  for (size_t i { 0 }; i < cnf.m; ++i) {
    gates[i].refs.reserve(cnf.clauses[i].size());
    for (auto const& lit : cnf.clauses[i]) {
      gates[i].refs.push_back(QRef { lit, QRefType::Var });
    }
  }
  gates.back().type = QGateType::And;
  for (size_t i { 0 }; i < cnf.m; ++i)
    gates.back().refs.push_back(
      QRef { static_cast<ssize_t>(i + 1), QRefType::Gate });
}

QCir qprime(QCir self) {
  ssize_t shift { self.vars.size() };
  self.vars.reserve(2 * self.vars.size());
  std::copy(std::begin(self.vars), std::end(self.vars), std::back_inserter(self.vars));
  for (auto& gate : self.gates)
    for (auto& ref : gate.refs)
      ref = shift_ref(ref, (ref.type == QRefType::Var) ? shift : 0);
  self.output = shift_ref(self.output, (self.output.type == QRefType::Var) ? shift : 0);
  return self;
}

QCir qneg(QCir self) {
  self.output.id = -self.output.id;
  return self;
}

QCir qand(QCir self, const QCir& other) {
  auto other_output { merge_cir(self, other) };
  self.gates.push_back(QGate { { self.output, other_output }, QGateType::And });
  self.output = { static_cast<ssize_t>(self.gates.size()), QRefType::Gate };
  return self;
}

QCir qor(QCir self, const QCir& other) {
  auto other_output { merge_cir(self, other) };
  self.gates.push_back(QGate { { self.output, other_output }, QGateType::Or });
  self.output = { static_cast<ssize_t>(self.gates.size()), QRefType::Gate };
  return self;
}

QCir qimply(QCir self, const QCir& other) {
  self.output.id = -self.output.id;
  self.gates.push_back(QGate { { self.output, merge_cir(self, other) }, QGateType::Or });
  self.output = { static_cast<ssize_t>(self.gates.size()), QRefType::Gate };
  return self;
}

static QRef shift_ref(const QRef& ref, ssize_t shift) {
  auto new_ref { ref };
  new_ref.id = ref.id + (ref.id < 0) ? -shift : shift;
  return new_ref;
}

static QRef merge_cir(QCir& left, const QCir& right) {
  if (left.vars.size() < right.vars.size())
    std::copy(std::begin(right.vars) + left.vars.size(), std::end(right.vars),
      std::back_inserter(left.vars));

  ssize_t shift { left.gates.size() };
  for (auto const& gate : right.gates) {
    std::vector<QRef> new_refs;
    new_refs.reserve(gate.refs.size());
    std::transform(std::begin(gate.refs), std::end(gate.refs),
      std::back_inserter(new_refs), [&shift](auto const& ref){
        return shift_ref(ref, (ref.type == QRefType::Gate) ? shift : 0);
      });
    left.gates.push_back({ new_refs, gate.type });
  }
  return shift_ref(right.output,
    (right.output.type == QRefType::Gate) ? shift : 0);
}

std::ostream &operator<<(std::ostream &os, const QCir &cir) {
  os << "#QCIR-13\n";
  for (size_t i { 0u }; i < cir.vars.size(); ++i) {
    if (i == 0 || cir.vars[i] != cir.vars[i-1]) {
        if (i > 0) os << ")\n";
        os << cir.vars[i] << "(";
    } else
        os << ", ";
    os << "v" << (i + 1);
  }
  if (cir.vars.size() > 0) os << ")\n";
  os << "output(" << cir.output << ")";
  for (size_t i { 0u }; i < cir.gates.size(); ++i) {
    os << "g" << (i + 1) << " = " << cir.gates[i] << "\n";
  }
  return os;
}

static std::ostream &operator<<(std::ostream &os, const QRef &ref) {
    os << ref.type << ref.id;
}

static std::ostream &operator<<(std::ostream &os, const QGate &gate) {
  os << gate.type << "(";
  for (size_t i { 0u }; i < gate.refs.size(); ++i) {
    if (i != 0u) os << ", ";
    os << gate.refs[i];
  }
  os << ")";
}

static std::ostream &operator<<(std::ostream &os, const QVarType &var_type) {
  switch (var_type) {
    case QVarType::Exists: return os << "exists";
    case QVarType::ForAll: return os << "forall";
    default: __builtin_unreachable();
  }
}

static std::ostream &operator<<(std::ostream &os, const QGateType &gate_type) {
  switch (gate_type) {
    case QGateType::And: return os << "and";
    case QGateType::Or: return os << "or";
    case QGateType::Xor: return os << "xor";
    case QGateType::Ite: return os << "ite";
    default: __builtin_unreachable();
  }
}

static std::ostream &operator<<(std::ostream &os, const QRefType &gate_type) {
  switch (gate_type) {
    case QRefType::Var: return os << "v";
    case QRefType::Gate: return os << "g";
    default: __builtin_unreachable();
  }
}