#ifndef _EXPRESSION_HPP
#define _EXPRESSION_HPP

#include "baselang/expression.hpp"

#include "expressions/operator.hpp"
#include "expressions/primitive.hpp"
#include "expressions/derivative.hpp"
#include "expressions/list.hpp"

// Calling functions; {a->b, a} -> b
class ApplyExp : public Differentiable {
    private:
        Expression *op;
        Expression **args;
    public:
        ApplyExp(Exp, Exp*);
        ~ApplyExp();

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
};

class DerivativeExp : public Expression {
    private:
        Expression *func;
        std::string var;
    public:
        DerivativeExp(Exp f, std::string s) : func(f), var(s) {}
        ~DerivativeExp() { delete func; }

        Value* valueOf(Env);

        Exp clone() { return new DerivativeExp(func->clone(), var); }
        std::string toString();
};

class FoldExp : public Expression {
    private:
        Expression *list;
        Expression *func;
        Expression *base;
    public:
        FoldExp(Expression *l, Expression *f, Expression *b)
                : list(l), func(f), base(b) {}
        ~FoldExp() { delete list; delete func; delete base; }
        
        Value* valueOf(Env);

        Exp clone() { return new FoldExp(list->clone(), func->clone(), base->clone()); }
        std::string toString();
};

class ForExp : public Differentiable {
    private:
        std::string id;
        Expression *set;
        Expression *body;
    public:
        ForExp(std::string x, Expression *xs, Expression *e) : id(x), set(xs), body(e) {}
        ~ForExp() { delete set; delete body; }
        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);

        Exp clone() { return new ForExp(id, set->clone(), body->clone()); }
        std::string toString();
};

// Condition expression that chooses paths
class IfExp : public Differentiable {
    private:
        Expression *cond;
        Expression *tExp;
        Expression *fExp;
    public:
        IfExp(Exp, Exp, Exp);
        ~IfExp() { delete cond; delete tExp; delete fExp; }

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone() { return new IfExp(cond->clone(), tExp->clone(), fExp->clone()); }
        std::string toString();
};

// Expression for defining variables
class LetExp : public Differentiable {
    private:
        std::string *ids;
        Expression **exps;
        Expression *body;
    public:
        LetExp(std::string*, Exp*, Exp);
        ~LetExp() {
            for (int i = 0; exps[i]; i++) delete exps[i];
            delete[] exps;
            delete body;
            delete[] ids;
        }

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
};

class MagnitudeExp : public Differentiable {
    private:
        Expression *exp;
    public:
        MagnitudeExp(Expression *e) : exp(e) {}
        ~MagnitudeExp() { delete exp; }
        Exp clone() { return new MagnitudeExp(exp->clone()); }

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);

        std::string toString();
};

class MapExp : public Differentiable {
    private:
        Expression *func;
        Expression *list;
    public:
        MapExp(Expression *l, Expression *f) : func(f), list(l) {}
        ~MapExp() { delete func; delete list; }
        Exp clone() { return new MapExp(list->clone(), func->clone()); }

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);
        
        std::string toString();
};

// Bool -> Bool expression that negates booleans
class NotExp : public Expression {
    private:
        Expression *exp;
    public:
        NotExp(Exp e) { exp = e; }
        ~NotExp() { delete exp; }

        Value* valueOf(Env);
        
        Exp clone() { return new NotExp(exp->clone()); }
        std::string toString();
};

class PrintExp : public Expression {
    private:
        Expression **args;
    public:
        PrintExp(Expression **l) : args(l) {}
        ~PrintExp() { for (int i = 0; args[i]; i++) delete args[i]; delete[] args; }

        Value* valueOf(Env);

        Exp clone();
        std::string toString();
};

class SequenceExp : public Expression {
    private:
        Expression *pre;
        Expression *post;
    public:
        SequenceExp(Exp, Exp = NULL);
        ~SequenceExp() { delete pre; delete post; }

        Value* valueOf(Env);
        
        Exp clone() { return new SequenceExp(pre->clone(), post->clone()); }
        std::string toString();
};

// Expression for redefining values in a store
class SetExp : public Differentiable {
    private:
        Expression **tgts;
        Expression **exps;
    public:
        SetExp(Exp*, Exp*);
        ~SetExp() {
            for (int i = 0; tgts[i]; i++) { delete tgts[i]; delete exps[i]; }
            delete[] tgts; delete[] exps;
        }

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);
        
        Exp clone();
        std::string toString();
};

class WhileExp : public Differentiable {
    private:
        Expression *cond;
        Expression *body;
        bool alwaysEnter; /* If true, always enter for at least one iteration; do-while */
    public:
        WhileExp(Expression *c, Expression *b, bool enter = false)
                : cond(c), body(b), alwaysEnter(enter) {}
        ~WhileExp() { delete cond; delete body; }

        Value* valueOf(Env);
        Value* derivativeOf(std::string, Env, Env);

        Exp clone() { return new WhileExp(cond->clone(), body->clone(), alwaysEnter); }
        std::string toString();
};

#endif
