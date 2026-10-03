/* A function prototype whose return type has more than one word, or a macro /
 * calling convention in front of it, is not the start of a variable definition. */

void Foo(void);
unsigned long Calc(int a, int b);
long long Other(void);
__forceinline int A(void);
__stdcall int B(void);
WINAPI int C(void);
__cdecl int D(void);
_Thread_local int E(void);
constexpr int F(void);
_Noreturn int G(void);
__declspec(dllexport) WINAPI int H(void);
__forceinline unsigned long I(void);
void Last(void);

/* real variable definitions with multi-word types are still variable blocks */
void Locals(void)
{
    foo();
    unsigned long x;
    MyMacro Widget w;
    unsigned long long y;
    int z;
    bar();
}

void Proto(void);
unsigned long gx;
MyMacro Widget gw;
void AfterProto(void);
