#define foo(expr) (expr)
using x = decltype          foo(int);
