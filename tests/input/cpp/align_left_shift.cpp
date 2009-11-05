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
}
