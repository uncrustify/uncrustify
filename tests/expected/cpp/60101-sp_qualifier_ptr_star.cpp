char* const*foo1 = (char* const *)"foo";
char* const  *foo2 = (char* const  *)"foo";

char* const*foo3(char* const *);
char* const  *foo4(char* const  *);

auto foo5() -> char* const*;
auto foo6() -> char* const  *;
