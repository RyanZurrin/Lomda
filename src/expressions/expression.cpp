#include "expression.hpp"

#include "value.hpp"

#include <string>

using namespace std;


void throw_warning(string form, string mssg) {
    std::cerr << "\x1b[31m\x1b[1m" << (form == "" ? "" : (form + " "))
            << "warning:\x1b[0m " << mssg << "\n";
}
void throw_err(string form, string mssg) {
    std::cerr << "\x1b[31m\x1b[1m" << (form == "" ? "" : (form + " "))
            << "error:\x1b[0m " << mssg << "\n";
}
void throw_type_err(Expression *exp, std::string type) {
    throw_err("runtime", "expression '" + exp->toString() + "' does not evaluate as " + type);
}



ApplyExp::ApplyExp(Expression *f, Expression **xs) {
    op = f;
    args = xs;
}

IfExp::IfExp(Expression *b, Expression *t, Expression *f) {
    cond = b;
    tExp = t;
    fExp = f;
}

IntExp::IntExp(int n) { val = n; }

// Expression for generating lambdas.
LambdaExp::LambdaExp(string *ids, Expression *rator) {
    xs = ids;
    exp = rator;
}

LetExp::LetExp(string *ids, Expression **xs, Expression *y) {
    this->ids = ids;
    exps = xs;
    body = y;
}

ListExp::ListExp(Expression** exps) : ListExp::ListExp() {
    for (int i = 0; exps[i]; i++)
        list->add(i, exps[i]);
}

OperatorExp::OperatorExp(Expression *a, Expression *b) {
    left = a;
    right = b;
}

RealExp::RealExp(float n) { val = n; }

SequenceExp::SequenceExp(Expression *a, Expression *b) {
    pre = a;
    post = b;
}

SetExp::SetExp(Expression **xs, Expression **vs) {
    tgts = xs;
    exps = vs;
}


