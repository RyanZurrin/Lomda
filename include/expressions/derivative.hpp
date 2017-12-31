#ifndef _EXPRESSIONS_DERIVATIVE_HPP_
#define _EXPRESSIONS_DERIVATIVE_HPP_

#include "baselang/expression.hpp"
#include "environment.hpp"

class Differentiable : public Expression {
    public:
        /**
         * Computes the derivative of an expression.
         * env - The known variable values.
         * denv - The known derivative values.
         */
        virtual Value* derivativeOf(std::string x, Env env, Env denv) = 0;
};


#endif
