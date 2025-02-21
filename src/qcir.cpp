#include "qcir.hpp"

#include <numeric>

std::ostream &operator<<(std::ostream &os, const QVarType &var_type);
std::ostream &operator<<(std::ostream &os, const QGateType &gate_type);
std::ostream &operator<<(std::ostream &os, const QRefType &ref_type);
std::ostream &operator<<(std::ostream &os, const QRef &ref);
std::ostream &operator<<(std::ostream &os, const QGate &gate);

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

std::ostream &operator<<(std::ostream &os, const QRef &ref) {
    os << ref.type << ref.id;
}

std::ostream &operator<<(std::ostream &os, const QGate &gate) {
  os << gate.type << "(";
  for (size_t i { 0u }; i < gate.refs.size(); ++i) {
    if (i != 0u) os << ", ";
    os << gate.refs[i];
  }
  os << ")";
}

std::ostream &operator<<(std::ostream &os, const QVarType &var_type) {
  switch (var_type) {
    case QVarType::Exists: return os << "exists";
    case QVarType::ForAll: return os << "forall";
    default: __builtin_unreachable();
  }
}

std::ostream &operator<<(std::ostream &os, const QGateType &gate_type) {
  switch (gate_type) {
    case QGateType::And: return os << "and";
    case QGateType::Or: return os << "or";
    case QGateType::Xor: return os << "xor";
    case QGateType::Ite: return os << "ite";
    default: __builtin_unreachable();
  }
}

std::ostream &operator<<(std::ostream &os, const QRefType &gate_type) {
  switch (gate_type) {
    case QRefType::Var: return os << "v";
    case QRefType::Gate: return os << "g";
    default: __builtin_unreachable();
  }
}