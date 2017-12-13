#ifndef _VALUE_HPP_
#define _VALUE_HPP_

#include "baselang/value.hpp"
#include "baselang/expression.hpp"
#include "baselang/environment.hpp"

class BoolVal : public Value {
    private:
        bool val;
    public:
        BoolVal(bool = 0);
        bool get();
        std::string toString();
        BoolVal* copy() { return new BoolVal(val); }
        int set(Value*);
};

class IntVal : public Value {
    private:
        int val;
    public:
        IntVal(int = 0);
        int get();
        std::string toString();
        IntVal* copy() { return new IntVal(val); }
        int set(Value*);
};

// Represents a closure
class LambdaVal : public Value {
    private:
        std::string *xs;
        Expression *exp;
        Environment *env;
    public:
        LambdaVal(std::string*, Expression*, Environment* = NULL);
        std::string toString();
        Value* apply(Value **xs, Environment *e = NULL);
        LambdaVal* copy() { return new LambdaVal(xs, exp, env->copy()); }
        int set(Value*);

        std::string* getArgs() { return xs; }

        Expression* getBody() { return exp; }
        
        Environment* getEnv() { return env; }
        void setEnv(Environment*);
};

class ListVal : public Value {
    // Note: it may be of interest to make this a subclass of
    // some form of map value in order to generalize certain
    // behaviors.
    private:
        List<Value*> *list;
    public:
        ListVal() { list = new LinkedList<Value*>(); }
        ListVal(List<Value*> *l) : list(l) {}

        List<Value*>* get() { return list; }
        ListVal* copy();
        int set(Value*);

        std::string toString();
};

class RealVal : public Value {
    private:
        float val;
    public:
        RealVal(float = 0);
        float get();
        int set(Value*);

        std::string toString();
        RealVal* copy() { return new RealVal(val); }
};

class VoidVal : public Value {
    public:
        std::string toString() { return "(void-val)"; }
        VoidVal* copy() { return new VoidVal; }
        int set(Value*) {}
};

#endif
