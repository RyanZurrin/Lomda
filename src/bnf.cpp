#include "bnf.hpp"

#include <cstddef>
#include <cstring>

#include "expression.hpp"

using namespace std;

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

inline bool is_id_char(char c, int i) {
    return
        (c >= 'A' && c <= 'Z')
    ||  (c >= 'a' && c <= 'z')
    ||  (c == '_') || (i && is_digit(c));
}

inline bool is_keyword(string s) {
    return s == "while" || s == "for" // Looping
        || s == "if" || s == "then" || s == "else" // Conditionals
        || s == "true" || s == "false" // Booleans
        || s == "lambda" // Lambda calculus
        || s == "d"  // Differential calculus
        || s == "insert" || s == "into" || s == "at"
        || s == "remove" || s == "from"
    ;
}

inline bool is_identifier(std::string s) {
    bool b = true;
    for (int i = 0; i < s.length() && b; i++)
        b = is_id_char(s[i], i);
    return b;
}

parsed_id parseId(string str) {
    int i = parseSpaces(str);
    int j;
    for (j = i; is_id_char(str[j], j-i); j++);

    //std::cout << "Searching for identifier in '" << str << "'\n";
    
    parsed_id res;
    if (j == i) {
        // Return an error
        res.len = -1;
        res.item = "";
    } else {
        // Formulate the result
        res.len = j;
        res.item = str.substr(i, j-i);

        if (is_keyword(res.item)) {
            // Identifiers should not be keywords
            res.len = -1;
            res.item = "";
        }
    }
    
    return res;
}

parsed_int parseInt(string str) {
    int i = parseSpaces(str);
    int j;
    int num = 0;
    int sign = 1;

    // Negate?
    if (str[i] == '-') {
        sign = -1;
        i++;
    }

    // Get the number
    for (j = i; str[j] >= '0' && str[j] <= '9'; j++)
        num = 10*num + (char) (str[j] - '0') * sign;

    struct parsed<int> res;
    if (i == j) {
        // Error
        res.len = -1;
        res.item = 0;
    } else {
        // Build the result
        res.len = j;
        res.item = num;
        //std::cout << " found integer: " << res.item << "\n";
    }

    return res;
}

int parseLit(string str, string lit) {
    //std::cout << "Searching for literal '" << lit << "' in '" << str << "'\n";

    int i = parseSpaces(str);

    if (i)
        str = str.substr(i);
    
    if (!strncmp(str.c_str(), lit.c_str(), lit.length())) {
        // Check to make sure that there is not a chained identifier
        if (is_identifier(lit) && is_id_char(str[lit.length()], 0))
            return -1;

        return i + lit.length();
    } else {
        return -1;
    }
}

int parseSpaces(string str) {
    int i;
    for (i = 0; str[i] == ' ' || str[i] == '\n' || str[i] == '\t'; i++);
    
    return i;
}

LinkedList<struct arglist>* parseArgList(string str, bool ends) {
    int i;
 
    // Now, we parse for argument lists
    LinkedList<struct arglist> *options = new LinkedList<struct arglist>;
    LinkedList<struct arglist> *arglists = new LinkedList<struct arglist>;
    
    // Initial condition: only a list with no arguments
    struct arglist alst;
    alst.list = new LinkedList<struct arg>;
    alst.len = 0;
    options->add(0, alst);
    
    while (!options->isEmpty()) {
        // Get a possible path
        alst = options->remove(0);
        
        // Get the string and parse for more arguments
        string s = str.substr(alst.len);
        ParsedPrgms ps = parsePemdas(s, false);
        
        while (!ps->isEmpty()) {
            parsed_prgm arg = ps->remove(0);
            string st = s.substr(arg.len);
            
            // Make a new argument list
            struct arglist lst;
            lst.list = new LinkedList<struct arg>;
            lst.len = alst.len + arg.len;

            // Add all of the items
            auto it = alst.list->iterator();
            while (it->hasNext())
                lst.list->add(lst.list->size(), it->next());
            
            // And the new one
            struct arg a;
            a.exp = arg.item;
            lst.list->add(lst.list->size(), a);

            // Now, we must check whether or not to actually add it
            if ((i = parseLit(st, ",")) >= 0) {
                // There is more to check
                lst.len += i;
                options->add(0, lst);
            } else {
                // The arglist is done
                arglists->add(0, lst);
            }
        }

        delete alst.list;
    }
    delete options;

    return arglists;

}

/**
 * <codeblk> ::= '{' <program> '}' | <statement>
 */
ParsedPrgms parseCodeBlock(string str, bool ends) {
    int i;

    //std::cout << "Searching for code block in '" << str << "'\n";

    // First, we check for programs with single line invocations
    ParsedPrgms bodies = parseStatement(str, ends);

    //std::cout << "found " << bodies->size() << " applicable statements\n";

    auto it = bodies->iterator(); while (it->hasNext()) {
        parsed_prgm p = it->next();
        //std::cout << " s: '" << *(p.item) << " (len: " << p.len << ")\n";
    }

    // Next, we check for multiline invocations
    if ((i = parseLit(str, "{")) >= 0) {
        // There is a subprogram to be found
        str = str.substr(i);
        int len = i;

        ParsedPrgms subps = parseProgram(str, false);

        auto it = subps->iterator();
        while (it->hasNext()) {
            parsed_prgm p = it->next();
            string s = str.substr(p.len);

            if ((i = parseLit(s, "}")) < 0) {
                delete p.item;
                continue;
            }
            
            s = s.substr(i);
            p.len += i + len;

            if (ends && parseSpaces(s) < s.length()) {
                delete p.item;
                continue;
            }

            // The body should be added
            bodies->add(0, p);
        }
        delete it, subps;
    }

    return bodies;
}

/**
 * <accessor-exp> ::= <apply-exp> | <list-access-exp> | <primitive-exp>
 */
ParsedPrgms parseAccessor(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;
    
    // The possible expansions
    ParsedPrgms lists = parsePrimitive(str, false);

    while (!lists->isEmpty()) {
        parsed_prgm lst = lists->remove(0);
        string s = str.substr(lst.len);
        
        if (!ends || parseSpaces(s) == s.length())
            res->add(0, lst);

        if (parseSpaces(s) == s.length()) {
            // The entire program was parsed, so we can stop here
            continue;
        }

        int i;
        if ((i = parseLit(s, "[")) >= 0) {
            // This is a list accessor
            lst.len += i;
            s = s.substr(i);

            ParsedPrgms indices = parsePemdas(s, false);
            while (!indices->isEmpty()) {
                parsed_prgm idx = indices->remove(0);

                string st = s.substr(idx.len);

                i = parseLit(st, "]");
                if (i < 0) { // Check for a slice expression
                    if ((i = parseLit(st, ":")) >= 0) {
                        st = st.substr(i);
                        idx.len += i;
                        
                        ParsedPrgms jndices = parsePemdas(st, false);
                        while (!jndices->isEmpty()) {
                            parsed_prgm jdx = jndices->remove(0);

                            string strn = st.substr(jdx.len);

                            i = parseLit(strn, "]");
                            if (i < 0) {
                                // Garbage collection
                                delete jdx.item;
                                continue;
                            }

                            jdx.len += idx.len + lst.len + i;
                            jdx.item = new ListSliceExp(lst.item->clone(), idx.item, jdx.item);

                            idx.item = idx.item->clone(); // For distinctiveness, clone the idx value

                            lists->add(0, jdx);
                        }
                        delete jndices;
                    } else
                        delete idx.item; // If it doesn't happen, the frontal index is not necessary
                    continue;
                };

                idx.len += i + lst.len;
                idx.item = new ListAccessExp(lst.item->clone(), idx.item);

                lists->add(0, idx);
            }
            delete indices;
        } else if ((i = parseLit(s, "(")) >= 0) {
            // This is a function accessor 
            lst.len += i;
            s = s.substr(i);

            LinkedList<struct arglist> *arglists = parseArgList(s, false);
            while (!arglists->isEmpty()) {
                struct arglist alst = arglists->remove(0);

                // Check the closing parenthesis
                string st = s.substr(alst.len);

                i = parseLit(st, ")");
                if (i < 0) {
                    // Garbage collection on the useless argument list
                    while (!alst.list->isEmpty())
                        delete alst.list->remove(0).exp;
                    delete alst.list;
                    continue;
                }

                st = st.substr(i);
                alst.len += i;

                Expression **args = new Expression*[alst.list->size() + 1];
                for (i = 0; !alst.list->isEmpty(); i++)
                    args[i] = alst.list->remove(0).exp;
                args[i] = NULL;
                delete alst.list; // The list is empty and useless (destroy it)

                parsed_prgm p;
                p.item = new ApplyExp(lst.item->clone(), args);
                p.len = alst.len + lst.len;

                lists->add(0, p);
            }

            delete arglists;
        }
    }
    delete lists;

    return res;
}

/**
 * <additive-exp> ::= <sum-exp> | <diff-exp> | <multiplicative-exp>
 */
ParsedPrgms parseAdditive(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    // First, find every multiplicative expression that could come before this one.
    // We won't require them to finish
    ParsedPrgms mult = parseMultiplicative(str, false);

    auto it = mult->iterator();
    while (it->hasNext()) {
        // Get the program branch
        parsed_prgm p = it->next();
        Expression *exp = p.item;
        int len = p.len;
        
        // The string to parse
        string s = str.substr(len);
        int i;

        if (parseSpaces(s) == s.length()) {
            // The parsed expression is complete
            res->add(0, p);
        } else if ((i = parseLit(s, "+")) >= 0) {
            // The operation is addition
            s = s.substr(i);
            len += i;

            // Get all of the possible subresults
            ParsedPrgms subps = parseAdditive(s, ends);
            
            // Then, add all of the possible outcomes
            auto pit = subps->iterator();
            while (pit->hasNext()) {
                parsed_prgm prog = pit->next();
                prog.item = new SumExp(exp, prog.item);
                prog.len += len;

                exp = exp->clone(); // For distinctiveness
                
                res->add(0, prog);
            }
        } else if ((i = parseLit(s, "-")) >= 0) {
            s = s.substr(i);
            len += i;

            // Get all of the possible subresults
            ParsedPrgms subps = parseAdditive(s, ends);
            
            // Then, add all of the possible outcomes
            auto pit = subps->iterator();
            while (pit->hasNext()) {
                parsed_prgm prog = pit->next();
                prog.item = new DiffExp(exp, prog.item);
                prog.len += len;

                exp = exp->clone(); // For distinctiveness
                
                res->add(0, prog);
            }
        } else if (!ends) {
            res->add(0, p);
        } else {
            delete exp;
        }
                
    }
    delete it;

    return res;
}

ParsedPrgms parseAndExp(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    // First, find every multiplicative expression that could come before this one.
    // We won't require them to finish
    ParsedPrgms mult = parseEquality(str, false);

    auto it = mult->iterator();
    while (it->hasNext()) {
        // Get the program branch
        parsed_prgm p = it->next();
        Expression *exp = p.item;
        int len = p.len;
        
        // The string to parse
        string s = str.substr(len);
        int i;

        if (parseSpaces(s) == s.length()) {
            // The parsed expression is complete
            res->add(0, p);
        } else if ((i = parseLit(s, "and")) >= 0) {
            // The operation is addition
            s = s.substr(i);
            len += i;

            // Get all of the possible subresults
            ParsedPrgms subps = parseAndExp(s, ends);
            
            // Then, add all of the possible outcomes
            auto pit = subps->iterator();
            while (pit->hasNext()) {
                parsed_prgm prog = pit->next();
                prog.item = new AndExp(exp, prog.item);
                prog.len += len;

                exp = exp->clone(); // For distinctiveness
                
                res->add(0, prog);
            }
        } else if (!ends) {
            res->add(0, p);
        } else delete exp;
    }
    delete it;

    return res;
}

/**
 * <derivative-exp> ::= 'd/d'<id> <statement>
 */
ParsedPrgms parseDerivative(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    int len = parseLit(str, "d/d");
    if (len < 0) return res;
    str = str.substr(len);
    
    // There should be exactly ZERO spaces following
    if (parseSpaces(str)) return res;
    
    // Then, check for the variable to differentiate around
    parsed_id var = parseId(str);
    if (var.len < 0) return res;

    len += var.len;
    str = str.substr(var.len);

    // Now, we parse for a body
    ParsedPrgms bodies = parseStatement(str, ends);

    while (!bodies->isEmpty()) {
        parsed_prgm p = bodies->remove(0);
        
        p.len += len;
        p.item = new DerivativeExp(p.item, var.item);

        res->add(0, p);

    }
    delete bodies;

    return res;
}

/**
 * <additive-exp> ::= <sum-exp> | <multiplicative-exp>
 */
ParsedPrgms parseEquality(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    ParsedPrgms mult = parseAdditive(str, false);

    auto it = mult->iterator();
    while (it->hasNext()) {
        // Get the program branch
        parsed_prgm p = it->next();
        Expression *exp = p.item;
        int len = p.len;
        
        // The string to parse
        string s = str.substr(len);
        int i;

        if (parseSpaces(s) == s.length()) {
            // The parsed expression is complete
            res->add(0, p);
        } else if (
                (i = parseLit(s, "==")) >= 0 ||
                (i = parseLit(s, "is")) >= 0 ||
                (i = parseLit(s, "equals")) >= 0
            ) {
            // The operation is addition
            s = s.substr(i);
            len += i;

            // Get all of the possible subresults
            ParsedPrgms subps = parseEquality(s, ends);
            
            // Then, add all of the possible outcomes
            auto pit = subps->iterator();
            while (pit->hasNext()) {
                parsed_prgm prog = pit->next();
                prog.item = new EqualsExp(exp, prog.item);
                prog.len += len;

                exp = exp->clone(); // For distinctiveness
                
                res->add(0, prog);
            }
        } else if (!ends) {
            res->add(0, p);
        } else {
            delete exp;
        }
                
    }

    return res;
}

/**
 * <lambda-exp> ::= 'lambda' '(' <args> ')' <codeblk>
 */
ParsedPrgms parseLambdaExp(string str, bool ends) {
    int len;
    ParsedPrgms res = new LinkedList<parsed_prgm>;
    
    // We will tolerate 'lambda' or the actual symbol
    // (for monsters who have the time to paste lambdas around)
    if ((len = parseLit(str, "lambda")) < 0)
    if ((len = parseLit(str, "λ")) < 0)
    return res;
    str = str.substr(len);
    
    // Check for the open parenthesis.
    int i = parseLit(str, "(");
    if (i < 0) return res;
    str = str.substr(i);
    len += i;

    // Parse a collection of identifiers
    LinkedList<string> xs;
    
    if ((i = parseLit(str, ")")) >= 0) {
        // The function takes zero arguments. So, we skip the search.
        str = str.substr(i);
        len += i;
    } else while (1) {
        
        parsed_id arg = parseId(str);
        if (arg.len < 0)
            // The argument set is invalid
            return res;
        else
            xs.add(xs.size(), arg.item);
        
        // Progress the string
        str = str.substr(arg.len);
        len += arg.len;

        if ((i = parseLit(str, ")")) >= 0) {
            // End of arguments
            str = str.substr(i);
            len += i;
            break;
        } else if ((i = parseLit(str, ",")) >= 0) {
            str = str.substr(i);
            len += i;
        } else {
            // Something is in place of the comma
            return res;
        }
    }
    
    // The possible code blocks
    ParsedPrgms bodies = parseCodeBlock(str, ends);

    // Now, we create a set of lambda-exps
    while (!bodies->isEmpty()) {
        parsed_prgm p = bodies->remove(0);

        string *ids = new string[xs.size()+1];
        auto xit = xs.iterator();
        for (i = 0; xit->hasNext(); i++) {
            ids[i] = xit->next();
        }
        ids[i] = "";
        
        // Add the item
        p.item = new LambdaExp(ids, p.item);
        p.len += len;
        res->add(0, p);
    }

    delete bodies;

    return res;
}

ParsedPrgms parseListExp(string str, bool ends) {
    //std::cout << "Searching for let-exp in '" << str << "'\n";

    ParsedPrgms res = new LinkedList<parsed_prgm>;
    int i;
    int len = parseLit(str, "[");
    if (len < 0) {
        // This is not a let expression
        return res;
    }
    str = str.substr(len);

    //std::cout << "perhaps a list: '" << str << "'\n";

    struct arglist lst;

    // Find every possible evaluation for argument lists.
    LinkedList<struct arglist> *lists = parseArgList(str, false);

    //std::cout << "list-exp candidates: " << lists->size() << "\n";
    
    // Then, verify each of them
    auto it = lists->iterator();
    while (it->hasNext()) {
        lst = it->next();
        string s = str.substr(lst.len);
        
        //std::cout << "Check for bracket in '" << s << "'\n";

        if ((i = parseLit(s, "]")) < 0) {
            // List candidate does not have closure
            while (!lst.list->isEmpty())
                delete lst.list->remove(0).exp;
            delete lst.list;
            continue;
        }
        
        lst.len += i;
        s = s.substr(i);

        if (ends && s.length() != parseSpaces(s)) {
            while (!lst.list->isEmpty())
                delete lst.list->remove(0).exp;
            delete lst.list;
            continue;
        }

        //std::cout << "List with " << lst.list->size() << " args\n";

        // Add to the list of possible expressions
        Expression **vals = new Expression*[lst.list->size() + 1];
        
        auto lit = lst.list->iterator();
        for (i = 0; lit->hasNext(); i++) {
            struct arg a = lit->next();
            vals[i] = a.exp;
        }
        vals[i] = NULL;
        delete lit, lst.list;

        // Here, we will modify a struct to make the changes.
        parsed_prgm prgm;
        prgm.len = lst.len + len;
        prgm.item = new ListExp(vals); // Collapse into target
        //std::cout << "Found list-exp '" << *(prgm.item) << "' (len: " << prgm.len << ")\n";
        
        // Add the result
        res->add(0, prgm);
     }

    // Garbage collection
    delete lists;

    return res;
}

/**
 * <multiplicative-exp> ::= <mult-exp> | <primitive>
 */
ParsedPrgms parseMultiplicative(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    // First, find every multiplicative expression that could come before this one.
    // We won't require them to finish
    ParsedPrgms mult = parseNotExp(str, false);

    auto it = mult->iterator();
    while (it->hasNext()) {
        // Get the program branch
        parsed_prgm p = it->next();
        Expression *exp = p.item;
        int len = p.len;
        
        // The string to parse
        string s = str.substr(len);
        int i;

        if (parseSpaces(s) == s.length()) {
            // The parsed expression is complete
            res->add(0, p);
        } else if ((i = parseLit(s, "*")) >= 0) {
            // The operation is addition
            s = s.substr(i);
            len += i;

            // Get all of the possible subresults
            ParsedPrgms subps = parseMultiplicative(s, ends);
            
            // Then, add all of the possible outcomes
            auto pit = subps->iterator();
            while (pit->hasNext()) {
                parsed_prgm prog = pit->next();
                prog.item = new MultExp(exp, prog.item);
                prog.len += len;

                exp = exp->clone(); // For distinctiveness
                
                res->add(0, prog);
            }
        } else if (!ends) {
            res->add(0, p);
        } else {
            delete exp;
        }
    }
    delete it;

    return res;
}

ParsedPrgms parseNotExp(string str, bool ends) {
    int i = parseLit(str, "not");
    if (i < 0) return parseAccessor(str, ends);

    ParsedPrgms res = new LinkedList<parsed_prgm>;

    str = str.substr(i);
    int len = i;

    // Get all of the possible subresults
    ParsedPrgms subps = parseNotExp(str, ends);
    
    // Then, add all of the possible outcomes
    auto pit = subps->iterator();
    while (pit->hasNext()) {
        parsed_prgm prog = pit->next();
        prog.len += len;
        prog.item = new NotExp(prog.item);
        res->add(0, prog);
    }

    return res;
}

ParsedPrgms parseParentheses(string str, bool ends) {
    ParsedPrgms res = new LinkedList<parsed_prgm>;

    int len = parseLit(str, "(");
    if (len < 0) return res;

    str = str.substr(len);

    ParsedPrgms statements = parsePemdas(str, false);

    while (!statements->isEmpty()) {
        parsed_prgm p = statements->remove(0);

        string s = str.substr(p.len);

        int i = parseLit(s, ")");
        if (i < 0) {
            delete p.item;
            continue;
        }
        
        p.len += len + i;
        s = s.substr(i);

        if (ends && s.length() != parseSpaces(s)) {
            delete p.item;
            continue;
        }

        res->add(0, p);

    }
    delete statements;

    return res;

}

/**
 * <primitive> ::= <lambda-exp> | <int-exp> | <var-exp> | <bool-exp>
 */
ParsedPrgms parsePrimitive(string str, bool ends) {
    ParsedPrgms res = parseLambdaExp(str, ends);

    // First, parse for an int-exp
    parsed_int num = parseInt(str);
    if (num.len >= 0 && (!ends || num.len + parseSpaces(str.substr(num.len)) == str.length())) {
        // Build and then insert the program structure
        parsed_prgm prgm;
        prgm.len = ends ? str.length() : num.len;
        prgm.item = new IntExp(num.item);
        res->add(0, prgm);

    }

    // Then, we parse for a var-exp
    parsed_id id = parseId(str);
    if (id.len > 0 && (!ends || id.len + parseSpaces(str.substr(id.len)) == str.length())) {
        // Build and then insert the program structure
        parsed_prgm prgm;
        prgm.len = ends ? str.length() : id.len;
        prgm.item = new VarExp(id.item);
        res->add(0, prgm);

    }

    // Parse for list-exp
    ParsedPrgms tmp = parseListExp(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;
    
    // Parse for true-exp
    int i = parseLit(str, "true");
    if (i > 0  && (!ends || i + parseSpaces(str.substr(i)) == str.length())) {
        parsed_prgm b; b.len = i; b.item = new TrueExp; res->add(0, b);
    }
    // Parse for false-exp
    i = parseLit(str, "false");
    if (i > 0 && (!ends || i + parseSpaces(str.substr(i)) == str.length())) {
        parsed_prgm b; b.len = i; b.item = new FalseExp; res->add(0, b);
    }
    
    // Parse for parenteses
    tmp = parseParentheses(str, ends);
    if (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    // Parse for derivative
    tmp = parseDerivative(str, ends);
    while (!tmp->isEmpty()) res->add(0, tmp->remove(0));
    delete tmp;

    return res;
}


