#include "stringable.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "environment.hpp"

using namespace std;

std::ostream &operator<< (std::ostream &os, Stringable &obj) {
    os << obj.toString();
    return os;
}

/* EXPRESSIONS */

string AndExp::toString() {
    return left->toString() + " and " + right->toString();
}
string ApplyExp::toString() {
    string s = (typeid(*op) == typeid(VarExp))
        ? op->toString() 
        : ("(" + op->toString() + ")");
    s += "(";
    for (int i = 0; args[i]; i++) {
        if (i) s += ", ";
        s += args[i]->toString();
    }

    s += ")";

    return s;
}
string CastExp::toString() {
    return exp->toString() + " as " + type;
}
string DerivativeExp::toString() {
    string s = "d/d" + var; 
    s += " " + (typeid(*func) == typeid(VarExp)
        ? func->toString()
        : "(" + func->toString() + ")"
    );
    return s;
}
string DictVal::toString() {
    string s = "{";

    auto kt = keys->iterator();
    auto vt = vals->iterator();

    for (bool b = false; kt->hasNext(); b = true) {
        if (b) s += ", ";
        s += kt->next() + " : " + vt->next()->toString();
    }

    s += "}";

    return s;
}
string DiffExp::toString() {
    return left->toString() + " - " + right->toString();
}
string DivExp::toString() {
    return left->toString() + " / " + right->toString();
}
string CompareExp::toString() {
    string s = left->toString();
    
    switch (operation) {
        case EQ:
            s += "=="; break;
        case NEQ:
            s += "!="; break;
        case GT:
            s += ">"; break;
        case LT:
            s += "<"; break;
        case GEQ:
            s += ">="; break;
        case LEQ:
            s += "<="; break;
    }

    return s + right->toString();
}
string FoldExp::toString() {
    return "fold " + list->toString() + " into " + func->toString() + " at " + base->toString();
}
string ForExp::toString() {
    return "for " + id + " in " + set->toString() + " " + body->toString();
}
string IfExp::toString() {
    return "if " + cond->toString() + " then " + tExp->toString() + " else " + fExp->toString();
}
string ImportExp::toString() {
    return "import " + module + (name == module ? "" : (" as " + name)) + (exp ? ("; " + exp->toString()) : "");
}
string IntExp::toString() {
    return to_string(val);
}
string IsaExp::toString() {
    return exp->toString() + " isa " + type;
}
string LambdaExp::toString() {
    string s = "lambda ("; 

    for (int i = 0; xs[i] != ""; i++) {
        if (i) s += ", ";
        s += xs[i];
    }

    s += ") ";
    
    if (typeid(*exp) == typeid(LetExp) || typeid(*exp) == typeid(SequenceExp))
        s += "(" + exp->toString() + ")";
    else
        s += exp->toString();

    return s;
}
string LetExp::toString() {
    string s = "let ";
    
    for (int i = 0; exps[i]; i++) {
        if (i) s += ", ";
        s += ids[i] + " = " + exps[i]->toString();
    }
    s += "; " + body->toString();
    return s;
}
string ListExp::toString() {
    string s = "[";

    Iterator<int, Expression*> *it = list->iterator();

    for (int i = 0; it->hasNext(); i++) {
        if (i) s += ", ";
        s += it->next()->toString();
    }

    s += "]";

    delete it;
    return s;

}

string ListAccessExp::toString() {
    return list->toString() + "[" + idx->toString() + "]";
}
string ListAddExp::toString() {
    return "insert " + elem->toString()
         + " into " + list->toString()
         + " at " + idx->toString();
}
string ListRemExp::toString() {
    return "remove " + idx->toString()
         + " from " + list->toString();
}
string ListSliceExp::toString() {
    return list->toString() + "["
            + (from ? from->toString() : "")
            + ":"
            + (to ? to->toString() : "") + "]";
}
string MagnitudeExp::toString() {
    return "|" + exp->toString() + "|";
}
string MapExp::toString() {
    return "map " + func->toString() + " over " + list->toString();
}
string MultExp::toString() {
    return left->toString() + " * " + right->toString();
}
string NormExp::toString() {
    return "||" + exp->toString() + "||";
}
string NotExp::toString() {
    return "not " + exp->toString();
}
string OrExp::toString() {
    return left->toString() + " or " + right->toString();
}
string PrintExp::toString() {
    string s = "print";
    for (int i = 0; args[i]; i++)
        s += " " + args[i]->toString();

    return s;
}
string RealExp::toString() {
    return to_string(val) + "f";
}
string SequenceExp::toString() {
    string s = "";

    auto it = seq->iterator();
    for (int i = 0; it->hasNext(); i++) {
        if (i) s += "; ";
        s += it->next()->toString();
    }
    
    return s;
}
string SetExp::toString() {
    return tgt->toString() + " = " + exp->toString();
}
string StdlibOpExp::toString() {
    string s = "";
    switch (op) {
        case SIN:
            s += "sin"; break;
        case COS:
            s += "sin"; break;
        case LOG:
            s += "log"; break;
        case SQRT:
            s += "sqrt"; break;
    }

    s += " " + x->toString();
    return s;
}
string StringExp::toString() {
    return "\"" + val + "\"";
}
string SumExp::toString() {
    return left->toString() + " + " + right->toString();
}
string ThunkExp::toString() {
    return "thunk { " + exp->toString() + " }";
}
string ValExp::toString() {
    return "antithunk { " + val->toString() + " }";
}
string VarExp::toString() {
    return id;
}
string WhileExp::toString() {
    if (alwaysEnter)
        return "do {\n" + body->toString() + "\n} while (" + cond->toString() + ")";
    else
        return "while (" + cond->toString() + ") {\n" + body->toString() + "\n}";
}

/* ENVIRONMENTS */

string EmptyEnv::toString() {
    return "{}";
}
string ExtendEnv::toString() {
    string s = "{" + id + " := ";
    if (typeid(*ref) == typeid(LambdaVal))
        s += "λ";
    else s += ref->toString();

    if (typeid(*subenv) == typeid(EmptyEnv))
        s += "}";
    else
        s += ", " + subenv->toString().substr(1);
    
    return s;
}

/* VALUES */

string BoolVal::toString() { return val ? "true" : "false"; }
string DictExp::toString() {
    string s = "{";

    auto kt = keys->iterator();
    auto vt = vals->iterator();

    for (bool b = false; kt->hasNext(); b = true) {
        if (b) s += ", ";
        s += kt->next() + " : " + vt->next()->toString();
    }

    s += "}";

    return s;
}
string IntVal::toString() { return to_string(val); }
string LambdaVal::toString() {
    string s =  "λ";

    for (int i = 0; xs[i] != ""; i++) {
        if (i) s += ",";
        s += xs[i];
    }
    
    s += "." + exp->toString();
    s += " | " + env->toString(); 

    return s;
}
string ListVal::toString() {
    string s = "[";

    for (int i = 0; i < list->size(); i++) {
        if (i) s += ", ";
        Val v = list->get(i);
        if (v)
            s += v->toString();
        else
            s += "null";
    }

    s += "]";
    return s;
}
string RealVal::toString() { return to_string(val); }
string StringVal::toString() { return val; }
string Thunk::toString() { return val ? val->toString() : ("" + exp->toString() + " | " + env->toString()); }

