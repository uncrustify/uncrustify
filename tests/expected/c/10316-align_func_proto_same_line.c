// Test: align_func_proto_span with several prototypes on one line:
// only the first one of a line is aligned, the others must not be glued
// to their return type

int         a(void); unsigned bb(void);
void        c(void); double dd(void);
long        e(void);

static int  f(int x)   { return x; }
static long gg(long y) { return y; }
int         h(int x)   { return x + 1; }

struct ops
{
    int (*open)(const char *path); int (*close)(int fd);
};

int x; long yy(void);
short z(void); int ww;

/* A newline in a parameter list does not end the line */
long  q(int a,
        int b); void r(void);
int   longer_name(void);

void  vars(void)
{
    int a = foo(1,
                2); int b;
    long cc = 3;
    int x2 = bar({1,
                  2}); int yy;
    long c3 = 4;
}
