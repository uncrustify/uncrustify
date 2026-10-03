// Issue #4777: a variable definition led by an attribute-like prefix is still a
// variable definition, so it takes part in alignment like its neighbours.

int a;
alignas(16) int bbbbbb;
int c;

int d;
_Alignas(16) int eeeeee;
int f;

int g;
__attribute__((aligned(16))) int hhhhhh;
int i;

int j;
__attribute__((a)) __attribute__((b)) int kkkkkk;
int l;

int m;
[[maybe_unused]] int nnnnnn;
int o;

int p;
__declspec(align(16)) int qqqqqq;
int r;

int s;
volatile int tttttt;
int u;

int v;
[[maybe_unused]] alignas(8) __attribute__((a)) int wwwwww;
int x;

int y;
static __declspec(align(16)) long zzzzzz;
int aa;

namespace N {
int a;
alignas(16) int bbbbbb;
int c;
}

class C {
    int a;
    __attribute__((aligned(16))) int bbbbbb;
    [[maybe_unused]] int cccccc;
    int d;
};

void func()
{
    int a;
    __attribute__((aligned(16))) int bbbbbb;
    alignas(16) int cccccc;
    [[maybe_unused]] int dddddd;
    int e;
}
