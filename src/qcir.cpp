#include "qcir.hpp"

#include <algorithm>
#include <numeric>

static QRef shift_ref(const QRef &ref, ssize_t shift);
static QRef merge_cir(QCir &left, const QCir &right);
static std::string to_string(const QVarType &var_type);
static std::string to_string(const QGateType &gate_type);
static std::string to_string(const QRef &ref, ssize_t gate_shift);
static std::string to_string(const QGate &gate, ssize_t gate_shift);

QCir::QCir(const CNF &cnf)
    : vars(2 * cnf.n, QVarType::Exists),
      gates(cnf.m + 1, QGate{{}, QGateType::Or}), output(0, QRefType::Gate) {
  for (size_t i{0}; i < cnf.m; ++i) {
    gates[i].refs.reserve(cnf.clauses[i].size());
    for (auto const &lit : cnf.clauses[i]) {
      gates[i].refs.push_back(QRef{lit, QRefType::Var});
    }
  }
  gates.back().type = QGateType::And;
  for (size_t i{0}; i < cnf.m; ++i)
    gates.back().refs.push_back(
        QRef{static_cast<ssize_t>(i + 1), QRefType::Gate});
  output.id = static_cast<ssize_t>(gates.size());
}

QCir to_qcir(const CNF &cnf) { return QCir(cnf); }

QCir qnext(QCir self) {
  const auto shift{static_cast<ssize_t>(self.vars.size() / 2)};
  for (auto &gate : self.gates)
    for (auto &ref : gate.refs)
      ref = shift_ref(ref, (ref.type == QRefType::Var) ? shift : 0);
  self.output =
      shift_ref(self.output, (self.output.type == QRefType::Var) ? shift : 0);
  return self;
}

QCir qneg(QCir self) {
  self.output.id = -self.output.id;
  return self;
}

QCir qand(QCir self, const QCir &other) {
  auto other_output{merge_cir(self, other)};
  self.gates.push_back(QGate{{self.output, other_output}, QGateType::And});
  self.output = {static_cast<ssize_t>(self.gates.size()), QRefType::Gate};
  return self;
}

QCir qor(QCir self, const QCir &other) {
  auto other_output{merge_cir(self, other)};
  self.gates.push_back(QGate{{self.output, other_output}, QGateType::Or});
  self.output = {static_cast<ssize_t>(self.gates.size()), QRefType::Gate};
  return self;
}

QCir qimply(QCir self, const QCir &other) {
  auto other_output{merge_cir(self, other)};
  self.output.id = -self.output.id;
  self.gates.push_back(QGate{{self.output, other_output}, QGateType::Or});
  self.output = {static_cast<ssize_t>(self.gates.size()), QRefType::Gate};
  return self;
}

static QRef shift_ref(const QRef &ref, ssize_t shift) {
  return {ref.id + ((ref.id < 0) ? -shift : shift), ref.type};
}

static QRef merge_cir(QCir &left, const QCir &right) {
  const auto shift{static_cast<ssize_t>(left.gates.size())};
  for (auto const &gate : right.gates) {
    std::vector<QRef> new_refs;
    new_refs.reserve(gate.refs.size());
    std::transform(std::begin(gate.refs), std::end(gate.refs),
                   std::back_inserter(new_refs), [&shift](auto const &ref) {
                     return shift_ref(ref,
                                      (ref.type == QRefType::Gate) ? shift : 0);
                   });
    left.gates.push_back({new_refs, gate.type});
  }
  return shift_ref(right.output,
                   (right.output.type == QRefType::Gate) ? shift : 0);
}

std::ostream &operator<<(std::ostream &os, const QCir &cir) {
  os << "#QCIR-G14 " << cir.vars.size() << "\n";
  size_t i{0u};
  for (; i < cir.vars.size(); ++i) {
    if (i == 0 || cir.vars[i] != cir.vars[i - 1]) {
      if (i > 0) os << ")\n";
      os << to_string(cir.vars[i]) << "(";
    } else
      os << ", ";
    os << (i + 1);
  }
  if (cir.vars.size() > 0) os << ")\n";
  os << "output(" << to_string(cir.output, cir.vars.size()) << ")\n";
  for (auto &gate : cir.gates) {
    os << (i++ + 1) << " = " << to_string(gate, cir.vars.size()) << "\n";
  }
  return os;
}

static std::string to_string(const QRef &ref, ssize_t gate_shift) {
  return ((ref.id < 0) ? "-" : "") +
         std::to_string(std::abs(ref.id) +
                        ((ref.type == QRefType::Gate) ? gate_shift : 0));
}

static std::string to_string(const QGate &gate, ssize_t gate_shift) {
  std::string result{to_string(gate.type)};
  result += "(";
  for (size_t i{0u}; i < gate.refs.size(); ++i) {
    if (i != 0u) result += ", ";
    result += to_string(gate.refs[i], gate_shift);
  }
  result += ")";
  return result;
}

static std::string to_string(const QVarType &var_type) {
  switch (var_type) {
  case QVarType::Exists: return "exists";
  case QVarType::ForAll: return "forall";
  default: __builtin_unreachable();
  }
}

static std::string to_string(const QGateType &gate_type) {
  switch (gate_type) {
  case QGateType::And: return "and";
  case QGateType::Or: return "or";
  case QGateType::Xor: return "xor";
  case QGateType::Ite: return "ite";
  default: __builtin_unreachable();
  }
}
