#include "interp.hpp"

#include "expression.hpp"
#include "value.hpp"
#include "environment.hpp"

#include "bnf.hpp"
#include "config.hpp"

#include "expressions/derivative.hpp"

#include <cstring>
#include <cstdlib>

using namespace std;

Val run(string program) {
    Exp exp = compile(program);
    if (exp) {
        // A tree was successfully parsed, so run it.
        Env env = new EmptyEnv();
        Val val = exp->valueOf(env);
        delete exp;
        delete env;
        //if (!val) throw_err("runtime", "could not evaluate expression");
        return val;
    } else {
        return NULL;
    }
}

Val ApplyExp::valueOf(Env env) {
    Val f = op->valueOf(env);

    // Null check the function
    if (!f)
        return NULL;
    
    // Type check the function
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(op, "lambda");
        return NULL;
    }
    LambdaVal *F = (LambdaVal*) f;

    int argc = 0;
    while (args[argc]) argc++;

    // Operate on each argument under the given environment
    Val *xs = new Val[argc+1];
    int i;
    for (i = 0; i < argc; i++) {
        // Evanuate the argument
        xs[i] = args[i]->valueOf(env);

        // Success test (every argument must successfully parse
        if (xs[i] == NULL) {
            // Garbage collect the list
            while (i--) xs[i]->rem_ref();
            delete xs;
            
            // And the function
            F->rem_ref();

            // An argument failed to parse, so clean up
            return NULL;
        }
    }
    xs[argc] = NULL;

    // Now, compute the answer under the lambda's environment
    Val y = F->apply(xs);
    
    // Garbage collection on the array
    while (i--) xs[i]->rem_ref();
    delete[] xs;

    // And the function
    F->rem_ref();

    return y;
}

Val DerivativeExp::valueOf(Env env) {

    // Base case: we know of nothing
    Env denv = new EmptyEnv;

    // Perform initial loading
    LinkedList<ExtendEnv*> env_frames;
    Env tmp = env;
    while (typeid(*tmp) == typeid(ExtendEnv)) {
        env_frames.add(0, (ExtendEnv*) tmp);
        tmp = tmp->subenvironment();
    }
    
    while (!env_frames.isEmpty()) {
        ExtendEnv *ee = env_frames.remove(0);
        Val v = ee->topVal();
        string id = ee->topId();
        
        if (typeid(*v) == typeid(LambdaVal)) {
            LambdaVal *lv = (LambdaVal*) v;
            int i;
            for (i = 0; lv->getArgs()[i] != ""; i++);

            string *ids = new string[i+1];
            ids[i] = "";
            while (i--) ids[i] = lv->getArgs()[i];

            LambdaVal *dv = new LambdaVal(ids, 
                            new DerivativeExp(
                                    lv->getBody()->clone(), var
                            ), lv->getEnv()->clone());

            // Lambda derivative: d/dx lambda (x) f(x) = lambda (x) d/dx f(x)
            // We will need the derivative for this
            denv = new ExtendEnv(id, dv, denv);
            dv->rem_ref(); // The derivative exists only within the environment
        } else {
            // Trivial derivative: d/dx c = 0, d/dx x = x
            int c = id == var ? 1 : 0;
            
            v = deriveConstVal(v, env->apply(var), c);

            if (v) {
                // The value is of a differentiable type
                denv = new ExtendEnv(id, v, denv);
                v->rem_ref(); // The derivative exists only within the environment
                throw_debug("calc_init", "d/d" + var + " " + id + " = " + v->toString());
            }
        }
    }

    // Now, we have the variable, the environment, and the differentiable
    // environment. So, we can simply derive and return the result.
    //std::cout << "compute d/d" << var << " | env ::= " << *env << ", denv ::= " << *denv << "\n";
    Val res = func->derivativeOf(var, env, denv);

    // Garbage collection
    delete denv;

    return res;
}

Val FalseExp::valueOf(Env env) { return new BoolVal(false); }

Val FoldExp::valueOf(Env env) {
    Val lst = list->valueOf(env);
    if (!lst) return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        lst->rem_ref();
        throw_type_err(list, "list");
        return NULL;
    }
    
    Val f = func->valueOf(env);
    if (!f) return NULL;
    else if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        lst->rem_ref();
        return NULL;
    }

    LambdaVal *fn = (LambdaVal*) f;
    int i;
    for (i = 0; fn->getArgs()[i] != ""; i++);
    if (i != 2) {
        throw_err("runtime", "function defined by '" + func->toString() + "' does not take exactly two arguments");
        lst->rem_ref();
        f->rem_ref();
        return NULL;
    }

    auto it = ((ListVal*) lst)->get()->iterator();
    Val xs[3];
    xs[0] = base->valueOf(env); // First slot is the accumulator
    xs[2] = NULL; // Last slot is a null terminator.

    while (it->hasNext() && xs[0]) {
        // Second slot is the element
        xs[1] = it->next();
        
        // Update the accumulator.
        Val acc = fn->apply(xs);
        xs[0]->rem_ref();
        xs[0] = acc;
    }
    
    delete it;

    fn->rem_ref();
    lst->rem_ref();

    return xs[0];
}

Val ForExp::valueOf(Env env) {
    // Evaluate the list
    Val listExp = set->valueOf(env);
    if (!listExp) return NULL;
    else if (typeid(*listExp) != typeid(ListVal)) {
        throw_type_err(set, "list");
        return NULL;
    }
    List<Val> *list = ((ListVal*) listExp)->get();
    
    // Gather an iterator
    Iterator<int, Val> *it = list->iterator();

    // Value to be return
    Val v = new VoidVal;

    while (it->hasNext()) {
        // Get the next item from the list
        Val x = it->next();
        
        // Build an environment
        Env e = new ExtendEnv(id, x, env->clone());

        v->rem_ref();
        v = body->valueOf(e);

    }

    delete it;

    return v;

}

Val IfExp::valueOf(Env env) {
    Val b = cond->valueOf(env);

    if (typeid(*b) != typeid(BoolVal)) {
        return NULL;
    }

    BoolVal *B = (BoolVal*) b;
    bool bRes = B->get();

    // Conditional garbage collection
    b->rem_ref();

    if (bRes)
        return tExp->valueOf(env);
    else
        return fExp->valueOf(env);
}

Val IntExp::valueOf(Env env) {
    return new IntVal(val);
}

Val LambdaExp::valueOf(Env env) {
    int argc;
    for (argc = 0; xs[argc] != ""; argc++);

    string *ids = new string[argc+1];
    ids[argc] = "";
    while (argc--) ids[argc] = xs[argc];

    return new LambdaVal(ids, exp->clone(), env->clone());
}

Val LetExp::valueOf(Env env) {
    // We will operate on a clone
    env = env->clone();

    int argc = 0;
    for (; exps[argc]; argc++);

    // I want to make all of the lambdas recursive.
    // So, I will track my lambdas for now
    LinkedList<LambdaVal*> lambdas;
    
    // Extend the environment
    for (int i = 0; i < argc; i++) {
        // Compute the expression
        Val v = exps[i]->valueOf(env);
        if (!v) {
            // Garbage collection will happen here
            delete env;
            return NULL;
        }
        
        // Add it to the environment
        Val x = v->clone();
        env = new ExtendEnv(ids[i], x, env);
        
        // Drop references
        v->rem_ref();
        x->rem_ref();

        // We permit all lambdas to have recursive behavior
        if (typeid(*x) == typeid(LambdaVal)) {
            lambdas.add(0, (LambdaVal*) x);
        }

    }
    
    // Apply recursive principles
    while (!lambdas.isEmpty()) {
        LambdaVal *v = lambdas.remove(0);
        v->setEnv(env->clone());

        // Although the environment contains itself, a self-reference
        // doesn't really count. In fact, it's a bitch to deal with.
        v->rem_ref();
    }

    // Compute the result
    Val y = body->valueOf(env);

    // Garbage collection
    delete env;
        
    // Return the result
    return y;

}

Val ListExp::valueOf(Env env) {
    // Generate a blank list
    ListVal *val = new ListVal;
    
    // Add each item
    auto it = list->iterator();
    for(int i = 0; it->hasNext(); i++) {
        // Compute the value of each item
        Val v = it->next()->valueOf(env);
        if (!v) {
            // Garbage collection on the iterator and the value
            delete it;
            delete val;

            return NULL;
        } else {
            val->get()->add(i, v);
        }
    }

    delete it;
    return val;
}

Val ListAccessExp::valueOf(Env env) {
    // Get the list
    Val f = list->valueOf(env);
    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    
    // The list
    List<Val> *vals = ((ListVal*) f)->get();
    
    // Get the index
    Val index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();

    // Get the item
    Val v = vals->get(i);
    v->add_ref();
    return v;

}

Val ListAddExp::valueOf(Env env) {
    // Get the list
    Val f = list->valueOf(env);
    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Val> *vals = ((ListVal*) f)->get();

    // Compute the index
    Val index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();

    // Compute the value
    Val val = elem->valueOf(env);
    if (!index)
        return NULL;
    
    // Remove and return the item
    vals->add(i, val);

    return new VoidVal;
}

Val ListRemExp::valueOf(Env env) {
    // Get the list
    Val f = list->valueOf(env);
    if (!f)
        return NULL;
    else if (typeid(*f) != typeid(ListVal)) {
        throw_type_err(list, "list");
        return NULL;
    }
    List<Val> *vals = ((ListVal*) f)->get();

    // Compute the index
    Val index = idx->valueOf(env);
    if (!index) return NULL;
    else if (typeid(*index) != typeid(IntVal)) {
        throw_type_err(idx, "integer");
        return NULL;
    }
    int i = ((IntVal*) index)->get();
    
    // Remove and return the item
    return vals->remove(i);
}

Val ListSliceExp::valueOf(Env env) {
    // Get the list
    Val lst = list->valueOf(env);
    if (!lst)
        return NULL;
    else if (typeid(*lst) != typeid(ListVal)) {
        throw_type_err(list, "list");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    
    // The list
    List<Val> *vals = ((ListVal*) lst)->get();

    // Get the index
    Val f = from->valueOf(env);
    if (!f) return NULL;
    else if (typeid(*f) != typeid(IntVal)) {
        throw_type_err(from, "integer");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    int i = ((IntVal*) f)->get();

    // Get the index
    Val t = to->valueOf(env);
    if (!t) return NULL;
    else if (typeid(*t) != typeid(IntVal)) {
        throw_type_err(to, "integer");
        lst->rem_ref(); // Garbage collection
        return NULL;
    }
    int j = ((IntVal*) t)->get();

    // Get the item
    LinkedList<Val> *vs = new LinkedList<Val>;
    
    auto it = vals->iterator();
    int x;
    for (x = 0; x < i; x++) it->next();
    for (;x < j && it->hasNext(); x++) {
        // Add the value and a reference to it
        Val v = it->next();
        vs->add(x-i, v);
        v->add_ref();
    }
    
    // Garbage collection
    lst->rem_ref();
    delete it;

    return new ListVal(vs);

}

Val MagnitudeExp::valueOf(Env env) {
    Val v = exp->valueOf(env);
    Val res = NULL;

    if (!v) return NULL;
    else if (typeid(*v) == typeid(IntVal)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        res = new IntVal(val > 0 ? val : -val);
    } else if (typeid(*v) == typeid(RealVal)) {
        // Magnitude of number is its absolute value
        int val = ((RealVal*) v)->get();
        res = new RealVal(val > 0 ? val : -val);
    } else if (typeid(*v) == typeid(ListVal)) {
        // Magnitude of list is its length
        int val = ((ListVal*) v)->get()->size();
        res = new IntVal(val > 0 ? val : -val);
    }
    
    // Garbage collection
    v->rem_ref();

    return res;
}

Val MapExp::valueOf(Env env) {
    Val vs = list->valueOf(env);
    if (!vs) return NULL;

    Val f = func->valueOf(env);
    if (!f) { vs->rem_ref(); return NULL; }
    
    if (typeid(*f) != typeid(LambdaVal)) {
        throw_type_err(func, "lambda");
        vs->rem_ref();
        return NULL;
    }

    // Extract the function... require that it have one argument
    LambdaVal *fn = (LambdaVal*) f;
    if (fn->getArgs()[0] == "" || fn->getArgs()[1] != "") {
        throw_err("runtime", "map function '" + fn->toString() + "' does not take exactly one argument");
        fn->rem_ref();
        vs->rem_ref();
        return NULL;
    }

    // Get the arguments
    Exp map = fn->getBody();

    if (typeid(*vs) == typeid(ListVal)) {
        // Given a list, map each element of the list
        ListVal *vals = (ListVal*) vs;
        ListVal *res = new ListVal();
        
        int i = 0;
        auto it = vals->get()->iterator();
        while (it->hasNext()) {
            Val v = it->next();

            Val *xs = new Val[2];
            xs[0] = v;
            xs[1] = NULL;

            Val elem = fn->apply(xs);

            delete[] xs;

            if (!elem) {
                fn->rem_ref();
                vals->rem_ref();
                res->rem_ref();
                return NULL;
            } else {
                res->get()->add(res->get()->size(), elem);
            }

            i++;
        }
        delete it;
        
        fn->rem_ref();
        vals->rem_ref();

        return res;

    } else if (WERROR()) {
        throw_err("runtime", "expression '" + list->toString() + " does not evaluate as list");
        vs->rem_ref();
        fn->rem_ref();
        return NULL;
    } else {
        throw_warning("runtime", "expression '" + list->toString() + " does not evaluate as list");

        Val xs[2];
        xs[0] = vs; xs[1] = NULL;

        Val v = fn->apply(xs);

        vs->rem_ref();
        fn->rem_ref();

        return v;
    }
}

Val sqnorm(Val v, Env env) {
    if (typeid(*v) == typeid(IntVal)) {
        // Magnitude of number is its absolute value
        int val = ((IntVal*) v)->get();
        return new IntVal(val * val);
    } else if (typeid(*v) == typeid(RealVal)) {
        // Magnitude of number is its absolute value
        int val = ((RealVal*) v)->get();
        return new RealVal(val * val);
    } else if (typeid(*v) == typeid(ListVal)) {
        // Magnitude of list is its length
        auto it = ((ListVal*) v)->get()->iterator();

        float sum = 0;
        
        while (it->hasNext()) {
            Val v = sqnorm(it->next(), env);

            if (!v) return NULL;

            auto x = typeid(*v) == typeid(IntVal)
                    ? ((IntVal*) v)->get()
                    : ((RealVal*) v)->get();
            v->rem_ref();

            sum += x;
        }

        return new RealVal(sum);

    } else return NULL;
}
Val NormExp::valueOf(Env env) {
    Val val = exp->valueOf(env);
    if (!val) return NULL;

    Val v = sqnorm(val, env);
    val->rem_ref();
    if (!v) return NULL;

    auto x = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();

    v->rem_ref();
    
    return x < 0 ? NULL : new RealVal(sqrt(x));
}


Val NotExp::valueOf(Env env) {
    Val v = exp->valueOf(env);
    
    if (!v)
        return NULL;
    else if (typeid(*v) != typeid(BoolVal)) {
        throw_type_err(exp, "boolean");
        v->rem_ref(); // Garbage
        return NULL;
    } else {
        BoolVal *B = (BoolVal*) v;
        bool b = B->get();
        
        // Garbage
        v->rem_ref();

        return new BoolVal(!b);
    }
}

Val OperatorExp::valueOf(Env env) {
    Val a = left->valueOf(env);
    if (!a) return NULL;

    Val b = right->valueOf(env);

    if (!b) {
        a->rem_ref();
        return NULL;
    } else {
        Val res = op(a, b);
        a->rem_ref();
        b->rem_ref();
        return res;
    }
}

Val PrintExp::valueOf(Env env) {
    string s = "";
    
    for (int i = 0; args[i]; i++) {
        Val v = args[i]->valueOf(env);
        if (!v)
            return NULL;

        if (i) s += " ";
        s += v->toString();
        
        v->rem_ref();
    }

    std::cout << s << "\n";
    return new VoidVal;
}

Val RealExp::valueOf(Env env) {
    return new RealVal(val);
}

Val SequenceExp::valueOf(Env env) {
    Val v = pre->valueOf(env);

    if (!v) return NULL;
    else if (!post) return v;
    else {
        v->rem_ref();
        return post->valueOf(env);
    }
}

Val SetExp::valueOf(Env env) {
    Val v = NULL;

    for (int i = 0; exps[i]; i++) {
        // Get info for modifying the environment
        Val u = tgts[i]->valueOf(env);
        if (!u)
            // The variable doesn't exist
            return NULL;

        // Evaluate the expression
        v = exps[i]->valueOf(env);

        if (!v)
            // The value could not be evaluated.
            return NULL;
        
        // Set the new value
        if (u->set(v))
            return NULL;

    }
    
    // To be simple, we return 0 on completion
    // as opposed to NULL, which indicates a failure.
    return v ? v : new VoidVal;
}

Val StdlibOpExp::valueOf(Env env) {
    Val v = x->valueOf(env);
    if (!v) return NULL;
    
    if (
        typeid(*v) == typeid(LambdaVal)
    ||  typeid(*v) == typeid(ListVal)
    ||  typeid(*v) == typeid(StringVal)
    ) {
        throw_type_err(x, "numerical");
    }

    auto z = typeid(*v) == typeid(IntVal)
            ? ((IntVal*) v)->get()
            : ((RealVal*) v)->get();
    v->rem_ref();
    
    switch (op) {
        case SIN: return new RealVal(sin(z));
        case COS: return new RealVal(cos(z));
        case LOG: return new RealVal(log(z));
        case SQRT: return new RealVal(sqrt(z));
    }
}

Val TrueExp::valueOf(Env env) { return new BoolVal(true); }

Val VarExp::valueOf(Env env) {
    Val res = env->apply(id);
    if (!res) throw_err("runtime", "variable '" + id + "' was not recognized");
    else res->add_ref(); // This necessarily creates a new reference. So, we must track it.

    return res;
}

Val WhileExp::valueOf(Env env) {
    bool skip = alwaysEnter;
    Val v = new VoidVal;

    while (true) {
        Val c = cond->valueOf(env);
        if (!c) return NULL;
        else if (typeid(*c) != typeid(BoolVal)) {
            throw_type_err(cond, "boolean");
            return NULL;
        } else if (skip || ((BoolVal*) c)->get()) {
            // If do-while was enacted, cease the action.
            skip = false;
            // Compute the new outcome. If it is
            // NULL, computation failed, so NULL
            // should be returned.
            if (!(v = body->valueOf(env)))
                return NULL;
        } else
            return v;
    }
}

