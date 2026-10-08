// A function prototype whose return type has more than one word, or a macro /
// calling convention in front of it, is not the start of a variable definition.

class TwoWords {
void Foo();
unsigned long Calc(int a, int b);
long long Other();
unsigned int More();
void Last();
};

class Prefixed {
void Foo();
__forceinline int A();
__stdcall int B();
WINAPI int C();
__cdecl int D();
thread_local int E();
consteval int F();
constinit int G();
_Noreturn int H();
__forceinline unsigned long I();
__forceinline Widget* J();
void Last();
};

class WithDeclspec {
void Foo();
__declspec(dllexport) WINAPI int A();
__declspec(dllexport) __forceinline int B();
[[nodiscard]] WINAPI int C();
void Last();
};

class Definitions {
void Foo() {
}
unsigned long Calc() {
    return 0;
}
__forceinline int A() {
    return 0;
}
void Last() {
}
};

namespace N {
void Foo();
unsigned long Calc();
WINAPI int A();
void Last();
}

void FileFoo();
unsigned long FileCalc();
__forceinline int FileA();
void FileLast();

// real variable definitions with multi-word types are still variable blocks
void Locals()
{
    foo();

    unsigned long x;
    MyMacro Widget w;
    unsigned long long y;
    int z;

    bar();
}

void Proto();

unsigned long gx;
MyMacro Widget gw;

void AfterProto();
