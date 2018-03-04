#include "types.hpp"
#include "expression.hpp"

using namespace std;

template<typename T>
inline bool isType(const Type* t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

TypeEnv::~TypeEnv() {
    for (auto it : types)
        delete it.second;

    for (auto it : mgu)
        delete it.second;
}

Type* TypeEnv::apply(string x) {
    if (types.find(x) == types.end()) {
        // We need to instantiate this type to be a new variable type
        auto V = new VarType(next_id);

        // Increment the next_id var
        int i;
        for (i = next_id.length()-1; i >= 0 && next_id[i] == 'z'; i--)
            next_id[i] = 'a';
        if (i < 0)
            next_id = "a" + next_id;

        types[x] = V;
        mgu[V->toString()] = V->clone();
    }
    return types[x]->clone();
}

int TypeEnv::set(string x, Type* v) {
    int res = 0;
    if (types.find(x) != types.end()) {
        delete types[x];
        res = 1;
    }
    types[x] = v;
    return res;
}

Tenv TypeEnv::clone() {
    Tenv env = new TypeEnv;
    for (auto it : types)
        env->set(it.first, it.second->clone());
    for (auto it : mgu)
        env->set_tvar(it.first, it.second->clone());
    return env;
}

Type* TypeEnv::get_tvar(string v) {
    if (mgu.find(v) != mgu.end())
        return mgu[v];
    else
        return NULL;
}

void TypeEnv::rem_tvar(string v) {
    if (mgu.find(v) != mgu.end()) {
        delete mgu[v];
        mgu.erase(v);
    }
}

string TypeEnv::toString() {
    string res = "{";
    int i = 0;
    for (auto it : types) {
        if (i) res += ", ";
        res += it.first + " := " + it.second->toString();
        i++;
    }
}

// Unification rules for fundamental type
Type* BoolType::unify(Type* t, Tenv tenv) {
    if (isType<BoolType>(t))
        return new BoolType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this, tenv);
}
Type* IntType::unify(Type* t, Tenv tenv) {
    if (isType<RealType>(t)) {
        return t->clone();
    } else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this, tenv);
}
Type* LambdaType::unify(Type* t, Tenv tenv) {
    if (isType<LambdaType>(t)) {
        LambdaType *other = (LambdaType*) t;

        auto x = left->unify(other->left, tenv);
        if (!x) return NULL;
        auto y = right->unify(other->right, tenv);
        if (!y) {
            delete x;
            return NULL;
        }

        // We now know that the types are both x -> y. Hence, we will change
        // the types.

        other->left = x;
        other->right = y;

        left = x->clone();
        right = y->clone();

        return new LambdaType(x, y);
    } else
        return NULL;
}
Type* ListType::unify(Type* t, Tenv tenv) {
    if (isType<ListType>(t))
        return type->unify(((ListType*) t)->type, tenv);
    else
        return t->unify(this, tenv);
}
Type* RealType::unify(Type* t, Tenv tenv) {
    if (isType<RealType>(t))
        return new RealType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this, tenv);
}
Type* TupleType::unify(Type* t, Tenv tenv) {
    if (isType<TupleType>(t)) {
        TupleType *other = (TupleType*) t;

        auto x = left->unify(other->left, tenv);
        if (!x) return NULL;
        auto y = right->unify(other->right, tenv);
        if (!y) {
            delete x;
            return NULL;
        }
        
        // We now know that the types are both x * y. Hence, we will change
        // the types.
        other->left = x;
        other->right = y;

        left = x->clone();
        right = y->clone();

        return new TupleType(x, y);
    } else
        return NULL;
}
Type* VarType::unify(Type* t, Tenv tenv) {
    Type *A = tenv->get_tvar(name);

    if (isType<VarType>(t)) {
        VarType *v = (VarType*) t;
        Type *B = tenv->get_tvar(v->toString());

        // Unify the two types
        auto x = A->unify(B, tenv);
        if (!x) return NULL;
        
        // Now, we need to update the variable values.
        tenv->set_tvar(v->name, x);
        tenv->set_tvar(name, x);

        return x;
    } else {
        // We must verify that this variable checks out
        auto T = A->unify(t, tenv);
        if (!T) return NULL;

        // Update the variable state
        tenv->set_tvar(name, T->clone());
         
        return T;
    }
}
Type* StringType::unify(Type* t, Tenv tenv) {
    if (isType<StringType>(t))
        return new VoidType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this, tenv);
}
Type* VoidType::unify(Type* t, Tenv tenv) {
    if (isType<VoidType>(t))
        return new VoidType;
    else if (isType<PrimitiveType>(t))
        return NULL;
    else
        return t->unify(this, tenv);
}

// Unification rules for operational types
Type* SumType::unify(Type* t, Tenv tenv) {
    if (isType<SumType>(t)) {
        SumType* other = (SumType*) t;

        auto x = left->unify(other->left, tenv);
        if (!x) return NULL;

        delete left;
        left = x;

        delete other->left;
        other->left = x->clone();

        auto y = right->unify(other->right, tenv);
        if (!y)
            return NULL;

        delete right;
        right = y;

        delete other->right;
        other->right = y->clone();;
        
        // Next, we can try unifying x and y
        auto z = x->unify(y, tenv);

        if (!z)
            // x and y have no unification. Hence, it is untypable.
            return NULL;
        else {
            delete x;
            delete y;
            left = z->clone();
            right = z->clone();
            other->left = z->clone();
            other->right = z->clone();
        }
        
        // We will now check to see if the result has been finalized; whether
        // or not this type is reducible to one of the two sides.
        x = z;
        while (isType<ListType>(x))
            x = ((ListType*) x)->subtype();

        if (isType<RealType>(x)) {
            // It has resolved to an nd array. Hence, the solution has been found.
            return z;
        } else if (isType<VarType>(x)) {
            delete other->right;
            other->right = z->clone();

            delete other->left;
            other->left = z->clone();

            delete right;
            other->right = z->clone();

            delete left;
            other->left = z->clone();
            
            return new SumType(z, z->clone());
        } else
            // There does not exist a unification
            return NULL;
    } else if (isType<RealType>(t)) {
        auto x = left->unify(t, tenv);
        if (!x) return NULL;

        delete left;
        left = x;
        
        x = right->unify(t, tenv);
        if (!x) return NULL;

        delete right;
        right = x;

        x = left->unify(right, tenv);
        if (!x) return NULL;

        delete left;
        delete right;

        left = x->clone();
        right = x->clone();

        auto z = x;
        while (isType<ListType>(x))
            x = ((ListType*) x)->subtype();

        if (isType<RealType>(x)) {
            // It has resolved to an nd array. Hence, the solution has been found.
            return z;
        } else if (isType<VarType>(x)) {
            return z;
        } else
            // There does not exist a unification
            return NULL;

    }
}

Type* MultType::unify(Type* t, Tenv tenv) {
    return NULL;
}

Type* IfExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    if (!C) return NULL;

    auto B = new BoolType;
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) return NULL;

    auto T = tExp->typeOf(tenv);
    auto F = fExp->typeOf(tenv);

    auto Y = T->unify(F, tenv);
    delete T;
    delete F;

    return Y;
}
Type* IsaExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;
    delete T;

    return new BoolType;
}
Type* LetExp::typeOf(Tenv tenv) {
    unordered_map<string, Type*> tmp;
    
    int i;
    for (i = 0; exps[i]; i++) {
        // We seek to define the type of the ith variable to be defined.
        auto t = exps[i]->typeOf(tenv);
        if (!t) {
            break;
        } else {
            auto s = tenv->apply(ids[i]);
            if (s) {
                // We may have to suppress variables.
                tmp[ids[i]] = s->clone();
            }
            tenv->set(ids[i], t);
        }
    }

    // Evaluate the body if possible
    auto T = exps[i] ? NULL : body->typeOf(tenv);
    
    // Restore the tenv
    for (auto it : tmp) {
        tenv->set(it.first, it.second);
        tmp[it.first] = NULL;
    }

    return T;
}
Type* SequenceExp::typeOf(Tenv tenv) {
    Type *T = NULL;

    auto it = seq->iterator();
    do {
        if (T) delete T;
        T = it->next()->typeOf(tenv);
    } while (it->hasNext() && T);

    delete it;
    return T;
}
Type* SetExp::typeOf(Tenv tenv) {
    // The target and expression should each have a type
    auto T = tgt->typeOf(tenv);
    if (!T) return NULL;
    auto E = exp->typeOf(tenv);

    Type *S = NULL;
    
    if (E) {
        // The type of the expression is the unification of
        // the two expressions
        S = T->unify(E, tenv);
        delete E;
    }
    
    delete T;
    
    return S;
}
Type* WhileExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    auto B = new BoolType;
    
    // Type the conditional
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) return NULL;
    else delete D;
    
    // Type the body
    D = body->typeOf(tenv);
    if (!D) return NULL;
    else delete D;

    return new VoidType;
}

Type* AndExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete Y;
        return NULL;
    }

    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        return NULL;
    }
    
    delete x;
    delete y;

    return B;
}
Type* OrExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete B;
        delete Y;
        return NULL;
    }

    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        delete B;
        return NULL;
    }
    
    delete x;
    delete y;

    return B;
}
Type* NotExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;

    auto B = new BoolType;

    auto x = T->unify(B, tenv);
    delete T;
    if (!x) {
        delete B;
        B = NULL;
    }
    
    delete x;

    return B;

}

// Typing rules that evaluate to type U + V
Type* SumExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    if (!A) return NULL;

    auto B = right->typeOf(tenv);
    if (!B) {
        delete A;
        return NULL;
    }
    
    // We must unify the two types
    auto C = A->unify(B, tenv);

    if (!C) return NULL;
    else if (isType<PrimitiveType>(C))
        return C;
    else {
        return new SumType(C, C->clone());
    }
}
Type* DiffExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    if (!A) return NULL;

    auto B = right->typeOf(tenv);
    if (!B) {
        delete A;
        return NULL;
    }
    
    // We must unify the two types
    auto C = A->unify(B, tenv);

    if (!C) return NULL;
    else if (isType<PrimitiveType>(C))
        return C;
    else {
        return new SumType(C, C->clone());
    }
}

Type* VarExp::typeOf(Tenv tenv) {
    return tenv->apply(id);
}


