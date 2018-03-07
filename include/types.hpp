#ifndef _TYPES_HPP_
#define _TYPES_HPP_

#include "baselang/types.hpp"
#include "value.hpp"

template<typename T>
inline bool isType(const Type* t) {
    return t && dynamic_cast<const T*>(t) != nullptr;
}

inline bool val_is_integer(Val v) {
    return typeid(*v) == typeid(IntVal);
}
inline bool val_is_real(Val v) {
    return typeid(*v) == typeid(RealVal);
}
inline bool val_is_number(Val v) {
    return val_is_integer(v) || val_is_real(v);
}


inline bool val_is_list(Val v) {
    return typeid(*v) == typeid(ListVal);
}
inline bool val_is_dict(Val v) {
    return typeid(*v) == typeid(DictVal);
}
inline bool val_is_tuple(Val v) {
    return typeid(*v) == typeid(TupleVal);
}
inline bool val_is_data_struct(Val v) {
    return val_is_list(v) || val_is_dict(v) || val_is_tuple(v);
}


inline bool val_is_bool(Val v) {
    return typeid(*v) == typeid(BoolVal);
}
inline bool val_is_lambda(Val v) {
    return typeid(*v) == typeid(LambdaVal);
}
inline bool val_is_string(Val v) {
    return typeid(*v) == typeid(StringVal);
}

class PairType : public Type {
    protected:
        Type *left;
        Type *right;
    public:
        PairType(Type *a, Type *b) : left(a), right(b) {}
        ~PairType() { delete left; delete right; }

        Type* getLeft() { return left; }
        Type* getRight() { return right; }

        bool isConstant(Tenv t) { return left->isConstant(t) && right->isConstant(t); }
};

// Composite primitive types
class LambdaType : public PairType {
    private:
        Tenv env;
    public:
        LambdaType(Type *a, Type *b, Tenv t = NULL)
            : PairType(a,b), env(t) {}
        ~LambdaType() { delete env; }

        Tenv getEnv() { return env; }
        void setEnv(Tenv e) { env = e; }

        Type* clone() { return new LambdaType(left->clone(), right->clone(), env ? env->clone() : NULL); }
        Type* unify(Type*, Tenv);
        Type* subst(Tenv tenv) { return new LambdaType(left->subst(tenv), right->subst(tenv)); }

        std::string toString();
};
class ListType : public Type {
    private:
        Type *type;
    public:
        ListType(Type *t) : type(t) {}
        ~ListType() { delete type; }
        Type* clone() { return new ListType(type->clone()); }
        Type* subtype() { return type; }

        Type* unify(Type*, Tenv);
        Type* subst(Tenv tenv) { return new ListType(type->subst(tenv)); }
        bool isConstant(Tenv t) { return type->isConstant(t); }


        std::string toString();
};
class TupleType : public PairType {
    public:
        using PairType::PairType;

        Type* clone() { return new TupleType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* subst(Tenv tenv) { return new TupleType(left->subst(tenv), right->subst(tenv)); }

        std::string toString();
};

/** Operator types
 * Motivation: We will wish to restrict the types of veriables
 */
class SumType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new SumType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* subst(Tenv tenv);

        std::string toString();
};
class MultType : public PairType {
    public:
        using PairType::PairType;
        Type* clone() { return new MultType(left->clone(), right->clone()); }
        Type* unify(Type*, Tenv);
        Type* subst(Tenv tenv);

        std::string toString();
};

// Primitive types
class PrimitiveType : public Type {};
class BoolType : public PrimitiveType {
    public:
        Type* clone() { return new BoolType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "B"; }
};
class RealType : public PrimitiveType {
    public:
        virtual Type* clone() { return new RealType; }
        virtual Type* unify(Type*, Tenv);
        virtual std::string toString() { return "R"; }
};
class IntType : public RealType {
    public:
        Type* clone() { return new IntType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "Z"; }
};
class StringType : public PrimitiveType {
    public:
        Type* clone() { return new StringType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "S"; }
};
class VoidType : public PrimitiveType {
    public:
        VoidType() {}
        Type* clone() { return new VoidType; }
        Type* unify(Type*, Tenv);
        std::string toString() { return "void"; }
};

// Special types for polymorphism
class VarType : public Type {
    private:
        std::string name;
    public:
        VarType(std::string v) : name(v) {}
        Type* clone() { return new VarType(name); }
        Type* unify(Type*, Tenv);
        Type* subst(Tenv tenv);

        bool isConstant(Tenv);

        std::string toString() { return name; }
};

#endif
