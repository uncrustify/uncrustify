/* Issue #4777: attribute-like prefixes (C23 [[...]], __attribute__, _Alignas,
 * __declspec, __unused) on prototypes and variable definitions. */

void Foo(void);
[[nodiscard]] int Bar(void);
[[deprecated("x")]] [[maybe_unused]] int Qux(void);
__attribute__((noinline)) int Quux(void);
__attribute__((visibility("default"))) __attribute__((a)) int Quuux(void);
__unused int Quuuux(void);
__declspec(dllexport) int Quuuuux(void);
[[nodiscard]] __declspec(dllexport) int Mixed(void);
void Baz(void);

int a;
_Alignas(16) int bbbbbb;
int c;

int d;
__attribute__((aligned(16))) int eeeeee;
int f;

int g;
[[maybe_unused]] int hhhhhh;
int i;

int j;
__declspec(align(16)) int kkkkkk;
int l;

int m;
[[maybe_unused]] _Alignas(8) __attribute__((a)) int nnnnnn;
int o;

void func(void)
{
    int a;
    __attribute__((aligned(16))) int bbbbbb;
    _Alignas(16) int cccccc;
    [[maybe_unused]] int dddddd;
    int e;
}
