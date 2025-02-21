#pragma once

#include <ostream>
#include <vector>

#include "cnf.hpp"

enum class QVarType { Exists, ForAll };
enum class QRefType { Gate, Var };
enum class QGateType { And, Or, Xor, Ite };
struct QRef {
    ssize_t id;
    QRefType type;
};
struct QGate {
    std::vector<QRef> refs;
    QGateType type;
};
struct QCir {
    QCir(const CNF& cnf);
    std::vector<QVarType> vars;
    std::vector<QGate> gates;
    QRef output;
};
std::ostream &operator<<(std::ostream &os, const QCir &cir);
