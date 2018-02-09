#include "value.hpp"
#include "environment.hpp"

using namespace std;

#include <string>

bool is_zero_val(Val e) {
    if (!e) return false;
    else if (typeid(*e) == typeid(IntVal))
        return ((IntVal*) e)->get();
    else if (typeid(*e) == typeid(RealVal))
        return ((RealVal*) e)->get();
    else if (typeid(*e) == typeid(ListVal)) {
        auto it = ((ListVal*) e)->get()->iterator();

        bool isZero = true;
        while (it->hasNext()) isZero = is_zero_val(it->next());

        return isZero;

    } else return false;
}

// Booleans
BoolVal::BoolVal(bool n) { val = n; }
bool BoolVal::get() { return val; }
int BoolVal::set(Val v) {
    if (typeid(*v) == typeid(BoolVal)) {
        val = ((BoolVal*) v)->val;
        return 0;
    } else return 1;
}

DictVal* DictVal::clone() {

    auto ks = new LinkedList<std::string>;
    auto vs = new LinkedList<Val>;
    
    // Keys
    auto kt = keys->iterator();
    while (kt->hasNext())
        ks->add(ks->size(), kt->next());
    delete kt;
    
    // Vals
    auto vt = vals->iterator();
    while (vt->hasNext()) {
        Val v = vt->next();
        v->add_ref();
        vs->add(vs->size(), v);
    }
    delete vt;
    
    // Build the result
    return new DictVal(ks, vs);
}
int DictVal::set(Val) { return 1; } // We will not allow setting of fields
Val DictVal::apply(string s) {
    auto kt = keys->iterator();
    auto vt = vals->iterator();

    while (kt->hasNext()) {
        string k = kt->next();
        Val v = vt->next();

        if (k == s) {
            delete kt;
            delete vt;

            return v;
        }
    }

    delete kt;
    delete vt;
    
    return NULL;
}

// Integers
IntVal::IntVal(int n) { val = n; }
int IntVal::get() { return val; }
int IntVal::set(Val v) {
    if (typeid(*v) == typeid(IntVal)) {
        val = ((IntVal*) v)->val;
        return 0;
    } else return 1;
}

// Decimals
RealVal::RealVal(float n) { val = n; }
float RealVal::get() { return val; }
int RealVal::set(Val v) {
    if (typeid(*v) == typeid(RealVal)) {
        val = ((RealVal*) v)->val;
        return 0;
    } else return 1;
}

int StringVal::set(Val v) {
    if (typeid(*v) == typeid(StringVal)) {
        val = ((StringVal*) v)->get();
        return 0;
    } else return 1;
}

// Lambdas
LambdaVal::LambdaVal(string *ids, Exp exp, Env env) {
    this->xs = ids;
    this->exp = exp;
    this->env = env;
}
int LambdaVal::set(Val v) {
    if (typeid(*v) == typeid(LambdaVal)) {
        LambdaVal *lv = (LambdaVal*) v;
        int i;
    
        // Allocate new memory for the input
        delete[] xs;
        for (i = 0; lv->xs[i] != ""; i++);
        
        // Set a new input set
        xs = new string[i+1];
        xs[i] = "";
        while (i--) xs[i] = lv->xs[i];
        
        // Set a new expression
        delete exp;
        exp = lv->exp->clone();
        
        // Set the environment
        Env e = env;
        env = ((LambdaVal*) v)->env;
        delete e;

        return 0;
    } else return 1;
}
LambdaVal::~LambdaVal() {
    delete[] xs;
    delete exp;
    env->rem_ref();
}
LambdaVal* LambdaVal::clone() {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--) ids[argc] = xs[argc];
    
    env->add_ref();
    return new LambdaVal(ids, exp->clone(), env);
}
Val LambdaVal::apply(Val *argv, Env e) {
    Env E = e ? e : env;

    // We will use a clone in order to preserve previously allocated memory blocks
    if (E)
        E->add_ref();
    else
        E = new EmptyEnv;

    // Verify that the correct number of arguments have been provided
    for (int i = 0; argv[i] || xs[i] != ""; i++)
        if ((!argv[i]) != (xs[i] == ""))
            // If one of the lists ends before the other, the input is bad.
            return NULL;
    
    // Create an environment for handling the processing
    for (int i = 0; argv[i]; i++)
        E = new ExtendEnv(xs[i], argv[i], E);

    // Compute the result
    Val res = exp->evaluate(E);
    
    // Garbage collection
    E->rem_ref();

    // Return it
    return res;
}
void LambdaVal::setEnv(Env e) {
    Env tmp = env;
    env = e;
    
    if (env) env->add_ref();
    if (tmp) tmp->rem_ref();
}

ListVal::~ListVal() {
    while (!list->isEmpty()) {
        Val v = list->remove(0);
        if (v) v->rem_ref();
    }
    delete list;
}
ListVal* ListVal::clone() {
    // Add a copy of each element of the list
    auto it = list->iterator();
    ListVal *res = new ListVal;
    
    for (int i = 0; it->hasNext(); i++) {
        Val v = it->next();
        res->list->add(i, v);
        if (v) v->add_ref();
    }

    // Give it back
    return res;
}
int ListVal::set(Val v) {
    if (typeid(*v) == typeid(ListVal)) {
        auto lst = list;

        list = new ArrayList<Val>;
        
        // Add each value to the new list
        auto vs = ((ListVal*) v)->list;
        auto it = vs->iterator();
        while (it->hasNext()) {
            // New references have been added
            auto v = it->next();
            v->add_ref();
            list->add(list->size(), v);
        }
        delete it;
        
        // Garbage collection
        while (!lst->isEmpty()) lst->remove(0)->rem_ref();
        delete lst;

        return 0;
    } else return 1;
}

int Thunk::set(Val v) {
    if (typeid(*v) == typeid(Thunk)) {
        Thunk *t = (Thunk*) v;
        
        if (val) val->rem_ref();
        delete exp;

        if (t->val) t->val->add_ref();
        
        exp = t->exp->clone();
        val = t->val;

        return 0;
    } else return 1;
}
Val Thunk::get(Env e) {
    if (!e) e = env;
    if (!val) {
        // Preemptive debug print
        throw_debug("thunk", "evaluating " + exp->toString() + " | " + e->toString());

        val = exp->evaluate(e);

        if (val && VERBOSITY()) std::cout << "\x1b[34m\x1b[1mthunk:\x1b[0m created new ref " << val << "\n";
    }

    // Debug print the outcome
    if (val) {
        val->add_ref();
        throw_debug("thunk ", "" + exp->toString() + " | " + e->toString() + " ↓ " + val->toString());
        return val;
    } else {
        throw_debug("thunk", "" + exp->toString() + " | " + e->toString() + " cannot be computed");
        return NULL;
    }
}

