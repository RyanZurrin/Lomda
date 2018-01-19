#ifndef _BASELANG_ENVIRONMENT_HPP_
#define _BASELANG_ENVIRONMENT_HPP_

#include "stringable.hpp"
#include "reffable.hpp"

#include "baselang/value.hpp"
#include "structures/list.hpp"


#include <iostream>
#include <cstddef>

typedef List<Value*> Store;

// Interface for environments.
class Environment : public Stringable, public Reffable {
    protected:
        // The majority of environments have subenvironments
        Environment* subenv = NULL;
    public:
        /**
         * Attempts to apply a variable name to an expression.
         *
         * @param x The variable to lookup.
         *
         * @return The value of the given variable, or NULL if it was not found.
         */
        virtual Val apply(std::string x) { return NULL; }

        /**
         * Reassigns the value of a variable.
         *
         * @param x The name of the variable.
         * @param v The value to assign to the variable (must be of the same type).
         *
         * @return Zero if the value is successfully assigned, otherwise non-zero.
         */
        virtual int set(std::string x, Val v) { return 1; }
        
        virtual void add_ref() {
            this->Reffable::add_ref();
            if (subenv) subenv->add_ref();
        }
        virtual void rem_ref() {
            this->Reffable::rem_ref();
            if (subenv) subenv->rem_ref();
        }

        virtual Environment* clone() = 0;
        
        /**
         * Gathers the child environment of this environment.
         *
         * @return The child environment.
         */
        Environment* subenvironment();
};
typedef Environment* Env;

#endif
