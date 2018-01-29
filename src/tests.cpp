#include "tests.hpp"

#include "interp.hpp"

#include <iostream>
using namespace std;

#include <unistd.h>

inline string txt_bold(string x) { return "\x1b[1m" + x + "\x1b[0m"; }

int testcase(string x, string y) {
    cout << txt_bold("Case ") + "'" + x + "': ";

    Val v = run(x);
    
    if (!v) {
        cout << "\x1b[31mFailed! (expected: " + y + ", threw error)\x1b[0m\n";
        return 1;
    } else if (v->toString() == y) {
        cout << "\x1b[32mSuccess!\x1b[0m\n";
        v->rem_ref();
        return 0;
    } else {
        cout << "\x1b[31mFailure (expected: " + y + ", got: " + v->toString() + ")\x1b[0m\n";
        v->rem_ref();
        return 1;
    }

}


int test() {
    int n = 0;

    string tests[] = {
        // Constant expressions
        "2", "2",
        "\"abc\"", "abc",

        // Booleans
        "true and false", "false",
        "not false", "true",
        
        // Lambdas
        "() -> 1", "λ.1 | {}",
        "lambda (x, y, z) x * y + z", "λx,y,z.x * y + z | {}",
        
        // Let-exp
        "let x = 2; x + 3", "5",
        "let f(x) = x*x; f(3)", "9",
    
        // Lists
        "[1, 2, 3, 4][2]", "3",
        
        // Loops
        "let x = 0; for i in [1, 2, 3] x = x + i; x", "6",
        "let x = 0, i = 1; while i <= 3 { x = x + i; i = i + 1 }; x", "6",

        // Explicit typing
        "\"123\" as int", "123",

    ""};

    for (int i = 0; tests[i] != ""; i += 2)
        n += testcase(tests[i], tests[i+1]);

    std::cout << "\n";

    if (n)
        cout << "\x1b[31m" << "Failed " << n << " test cases\n";
    else
        cout << "\x1b[32m" << "All test cases passed!\n";

    cout << "\x1b[0m";
    
    return n ? 1 : 0;

}


