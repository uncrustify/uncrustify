// Issue #4777: nl_before_func_* must add the blank lines before the start of
// the declaration, i.e. before the attribute-like prefix, not after it.

int x;
[[nodiscard]] int P1();
int y;
__attribute__((noinline)) int P2();
int z;
__declspec(dllexport) int P3();
int w;
alignas(16) int P4();
int v;
__unused int P5();
int u;
[[a]] __declspec(dllexport) __attribute__((b)) int P6();
int t;
__declspec(dllexport) WINAPI int P7();
int s;
static int P8();
int r;
__attribute__((noinline)) int D1() { return 0; }
int q;
[[nodiscard]] int D2() { return 0; }
int p;
__declspec(dllexport) int D3() { return 0; }
int o;
alignas(16) int D4() { return 0; }
int n;

class C {
    int m;
    [[nodiscard]] int P1();
    int l;
    __attribute__((noinline)) int P2();
    int k;
    __declspec(dllexport) int P3();
    int j;
    [[nodiscard]] friend int P4();
    int i;
    friend int P5();
    int h;
    friend __declspec(dllexport) int P6();
    int g;
    [[nodiscard]] int D1() { return 0; }
    int f;
};
