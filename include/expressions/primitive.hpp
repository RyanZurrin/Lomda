#ifndef _EXPRESSIONS_PRIMITIVE_HPP_
#define _EXPRESSIONS_PRIMITIVE_HPP_

#include "expressions/derivative.hpp"
#include "linalg/matrix.hpp"

// Generates false
class FalseExp : public Expression {
    public:
        FalseExp() {}
        Value* valueOf(Environment*);
        
        Expression* clone() { return new FalseExp(); }
        std::string toString() { return "false"; }
};

// Expression for an integer.
class IntExp : public Differentiable {
    private:
        int val;
    public:
        IntExp(int = 0);
        Value* valueOf(Environment*);
        Value* derivativeOf(std::string, Environment*, Environment*) { return new IntVal(0); } // d/dx c = 0
        
        Expression* clone() { return new IntExp(val); }
        std::string toString();
};

// Expression that yields closures (lambdas)
class LambdaExp : public Differentiable {
    private:
        std::string *xs;
        Expression *exp;
    public:
        LambdaExp(std::string*, Expression*);
        ~LambdaExp() { delete[] xs; delete exp; }
        Value* valueOf(Environment*);
        Value* derivativeOf(std::string, Environment*, Environment*);

        std::string *getXs() { return xs; }
        
        Expression* clone();
        std::string toString();
};

class ListExp : public Differentiable {
    private:
        List<Expression*> *list;
    public:
        ListExp() : list(new LinkedList<Expression*>) {}
        ~ListExp() {
            while (!list->isEmpty()) delete list->remove(0);
            delete list;
        }

        ListExp(Expression**);
        ListExp(List<Expression*>* l) : list(l) {}
        Value* valueOf(Environment*);
        Value* derivativeOf(std::string, Environment*, Environment*);
        
        Expression* clone();
        std::string toString();
};

class MatrixExp : public Expression {
    private:
        Expression *list;
    public:
        MatrixExp(Expression *m) : list(m) {};
        ~MatrixExp() { delete list; }

        Value* valueOf(Environment*);

        Expression* clone() { return new MatrixExp(list->clone()); }
        std::string toString();
};

// Expression for a real number.
class RealExp : public Differentiable {
    private:
        float val;
    public:
        RealExp(float = 0);
        Value* valueOf(Environment*);
        Value* derivativeOf(std::string, Environment*, Environment*) { return new IntVal(0); } // d/dx c = 0
        
        Expression* clone() { return new RealExp(val); }
        std::string toString();
};

// Generates true
class TrueExp : public Expression {
    public:
        TrueExp() {}
        Value* valueOf(Environment*);
        
        Expression* clone() { return new TrueExp(); }
        std::string toString() { return "true"; }
};

// Get the value of a variable
class VarExp : public Differentiable {
    private:
        std::string id;
    public:
        VarExp(std::string s) : id(s) {}
        Value* valueOf(Environment *env);
        Value* derivativeOf(std::string, Environment*, Environment*);
        
        Expression* clone() { return new VarExp(id); }
        std::string toString() { return id; }
};

#endif
