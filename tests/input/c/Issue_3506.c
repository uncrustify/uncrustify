#define CONCAT2x(a,b) a ## _ ## b
#define CONCAT2(a,b) CONCAT2x(a,b)

typedef struct S {
int a;
int b;
} S;

static const S CONCAT2(foo, bar) = {
3,
4
};
