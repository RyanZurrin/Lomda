#ifndef _EXPRESSIONS_PRIMITIVE_HPP_
#define _EXPRESSIONS_PRIMITIVE_HPP_

#include "baselang/expression.hpp"
#include "value.hpp"
#include "linalg/matrix.hpp"

// Generates false
class FalseExp : public Expression {
    public:
        FalseExp() {}
        Val valueOf(Environment*);
        
        Exp clone() { return new FalseExp(); }
        std::string toString() { return "false"; }
};

// Expression for an integer.
class IntExp : public Expression {
    private:
        int val;
    public:
        IntExp(int = 0);
        Val valueOf(Environment*);
        Val derivativeOf(std::string, Environment*, Environment*);
        
        Exp clone() { return new IntExp(val); }
        std::string toString();
};

// Expression that yields closures (lambdas)
class LambdaExp : public Expression {
    private:
        std::string *xs;
        Exp exp;
    public:
        LambdaExp(std::string*, Exp);
        ~LambdaExp() { delete[] xs; delete exp; }
        Val valueOf(Environment*);
        Val derivativeOf(std::string, Environment*, Environment*);

        std::string *getXs() { return xs; }
        
        Exp clone();
        std::string toString();

        Exp optimize() { exp = exp->optimize(); return this; }
        Exp opt_const_prop(std::unordered_map<std::string, Exp> vs) 
                { exp = exp->opt_const_prop(vs); return this; }
};

class ListExp : public Expression {
    private:
        List<Exp> *list;
    public:
        ListExp() : list(new LinkedList<Exp>) {}
        ~ListExp() {
            while (!list->isEmpty()) delete list->remove(0);
            delete list;
        }

        ListExp(Exp*);
        ListExp(List<Exp>* l) : list(l) {}
        Val valueOf(Environment*);
        Val derivativeOf(std::string, Environment*, Environment*);
        
        Exp clone();
        std::string toString();
};

// Expression for a real number.
class RealExp : public Expression {
    private:
        float val;
    public:
        RealExp(float = 0);
        Val valueOf(Environment*);
        Val derivativeOf(std::string, Environment*, Environment*);
        
        Exp clone() { return new RealExp(val); }
        std::string toString();
};

class StringExp : public Expression {
    private:
        std::string val;
    public:
        StringExp(std::string s) : val(s) {}

        Val valueOf(Environment*) { return new StringVal(val); }

        Exp clone() { return new StringExp(val); }
        std::string toString();
};

// Generates true
class TrueExp : public Expression {
    public:
        TrueExp() {}
        Val valueOf(Environment*);
        
        Exp clone() { return new TrueExp(); }
        std::string toString() { return "true"; }
};

// Get the value of a variable
class VarExp : public Expression {
    private:
        std::string id;
    public:
        VarExp(std::string s) : id(s) {}

        Val valueOf(Environment *env);
        
        Exp clone() { return new VarExp(id); }
        std::string toString();

        Val derivativeOf(std::string, Environment*, Environment*);

        /**
         * Constant propagation will trivially replace the variable if
         * the variable name matches.
         */
         Exp opt_const_prop(std::unordered_map<std::string, Exp>&);
};

#endif
