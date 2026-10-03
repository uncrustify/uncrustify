/* Issue #4777: nl_before_func_* adds the blank lines before the start of the
 * declaration, i.e. before an attribute-like prefix. */

int x;
[[nodiscard]] int P1(void);
int y;
__attribute__((noinline)) int P2(void);
int z;
__declspec(dllexport) int P3(void);
int w;
__unused int P4(void);
int v;
[[a]] __declspec(dllexport) __attribute__((b)) int P5(void);
int u;
__declspec(dllexport) WINAPI int P6(void);
int t;
static int P7(void);
int s;
__attribute__((noinline)) int D1(void) { return 0; }
int r;
[[nodiscard]] int D2(void) { return 0; }
int q;
__declspec(dllexport) int D3(void) { return 0; }
int p;
