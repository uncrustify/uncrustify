#include <stdio.h>

char* const*foo1 = (char* const*)"foo";
char* const  *foo2 = (char* const  *)"foo";

char * const*foo3 = (char* const*)"foo";
char   * const  *foo4 = (char* const  *)"foo";
char   * const  *foo5 = (char* const  *)"foo";
char*const*foo6 = (char*const*)"foo";

char* const*foo7(char* const*);
char* const  *foo8(char* const  *);

int*const *bar0(int);
char*const*bar1(char* const*);
char*const  *bar2(char* const  *);
char  *const  *bar3(char* const  *);
char *const  *bar4(char* const  *);

void *operator new(int size);
void*operator new(int size);
void*    operator new(int size);
void     *operator new(int size);
void* operator new(int size);

void*::alloc1(int size);
void*::alloc2(int size);
void* ::alloc3(int size);
void* ::alloc4(int size);
void* ::alloc5(int size);
void*     ::alloc6(int size);


class Test {
void* method1();
void *method2();
void  *method3();
void*  method4();
void  *  method5();
};

void* Test::method1() {
	return nullptr;
}
void *Test::method2() {
	return NULL;
}
void  *Test::method3() {
	return NULL;
}
void*  Test::method4() {
	return nullptr;
}
void  *  Test::method5() {
	return 0;
}


namespace testing {

int *func1();
int*func2();
int *func3();
int*func4();
int*func5();

namespace test {
int *foo();
}

}

int *testing::func1() {
	return nullptr;
}
int* testing::func2() {
	return NULL;
}
int   *testing::func3() {
	return nullptr;
}
int * testing::func4() {
	return NULL;
}
int  *  testing::func5() {
	return NULL;
}

int*testing::test::foo()
{
	return NULL;
}

namespace Bar {

class Foo {
int*operator[] (int x) {
}

Foo *get();
}

}

Bar::Foo*Bar::Foo::get() {
}

void* not_malloc(const int);

void*::not_malloc(const int n)
{
}

int main()
{
	void (* name)();

	int *const *x;
	int *const*x;
	(int* )x;
	(const int *const)x;
	(int const*)x;
	(int const *)x;
	int y = 2+*x;
	return 0;
}

auto baz1() -> char* const*;
auto baz2() -> char* const  *;
auto baz3() -> char * const  *;
auto baz4() -> char*const  *;
