#define MEM_ASSERT1(x) if (!(x)) *(volatile int *)0 = 1
#define MEM_ASSERT2(x) if (!(x)) *(volatile int *)0 = 1
#define MEM_ASSERT3(x) if (!(x)) *(volatile int *)0 = 1;
#define MEM_ASSERT4(x) if (!(x)) *(volatile int *)0 = 1;
#define MEM_ASSERT5(x) if (!(x)) { *(volatile int *)0 = 1; }
#define MEM_ASSERT6(x) if (!(x)) { *(volatile int *)0 = 1; }

#define FOO1(x) while (!(x)) { *(volatile int *)0 = 1; }
#define FOO2(x) while (!(x)) *(volatile int *)0 = 1;
#define FOO3(x) { *(volatile int *)0 = 1; }
#define FOO4(x) *(volatile int *)0 = 1;
#define FOO5(x) for(;;) (!(x)) { *(volatile int *)0 = 1; }
#define FOO6(x) for(;;) (!(x)) *(volatile int *)0 = 1;
#define FOO7(x) do { *(volatile int *)0 = 1; } while (false);

void foo1(int x) {
    if (!(x)) *(volatile int *)0 = 1;
}

void foo2(int x) {
    if (!(x)) *(volatile int *)0 = 1;
}

void foo3(int x) {
    if (!(x)) { *(volatile int *)0 = 1; }
}

void foo4(int x) {
    if (!(x)) { *(volatile int *)0 = 1; }
}
