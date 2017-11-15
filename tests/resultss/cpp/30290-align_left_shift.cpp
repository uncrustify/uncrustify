#include <iostream>
#define MACRO(x) x
int main()
{
    int X[1];
    MACRO(std::cout << X
            << X[0]);
    std::cout << X
        << X;
    std::cout2 << X
        << X;
    std::cout << X
        << X[0];
    std::cout <<
        X <<
        Y;
    std::cout
        << X
        << Y;
    std::cout
        <<
        X
        <<
        Y;
}

#define A_LONG_MACRO_NAME(x) x

void f() {
    std::cout << "Hello, "
        << "World!"
        << std::endl;
    A_LONG_MACRO_NAME(std::cout << "Hello, "
            << "World!"
            << std::endl);
    A_LONG_MACRO_NAME(
	    std::cout << "Hello, "
	    << "World!"
	    << std::endl);
}

