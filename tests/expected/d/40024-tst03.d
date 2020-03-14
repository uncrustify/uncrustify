import std.stdio;

void foo()
{
    float  f = x % y;
    double d = x % y;
    real   r = x % y;

    result = (x < y);    // false if x or y is nan
    assert(e == 0);
    int array[17];

    array[] = value;
    int array[17];

    for (i = 0; i < array.length; i++)
        func(array[i]);
    int array[17];

    foreach (int value; array)
        func(value);
    int[] array;

    array.length            = array.length + 1;
    array[array.length - 1] = x;
    char[] s1;
    char[] s2;
    char[] s;

    s = s1 ~ s2;
    s ~= "hello";


    writefln("Calling all cars %s times!", ntimes);
 Louter:
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            if (j == 3)
                break Louter;
            if (j == 4)
                continue Louter;
        }
    }
    // break Louter goes here
    typedef bit    bHandle;
    typedef int    iHandle = -1;
    typedef void   *Handle = cast(void *)(-1);
    Handle h;

    h = func();
    if (h != Handle.init)
        ;
    char[] string = "hello";

    if (string < "betty")
        ;
    int *p = address;

    volatile { i = *p; }

    char[] ml = "This text spans
multiple
lines
";
}

void dostring(char[] s)
{
    switch (s)
    {
    case "hello":;

    case "goodbye":;

    case "maybe":;

    default:;
    }
}

struct ABC
{
    int           z;    // z is aligned to the default

    align (1) int x;    // x is byte aligned
    align (4)
    {
        ...             // declarations in {} are dword aligned
    }
    align (2) :         // switch to word alignment from here on

    int y;              // y is word aligned
}

struct Foo
{
    int i;
    union
    {
        struct { int x; long y; }
        char *p;
    }
}

struct Foo { int x; int y; }    // note there is no terminating ;
Foo foo;


struct Foo { int x; int y; }

off = Foo.y.offsetof;

union U { int a; long b; }
U x = { a:5 };

struct S { int a; int b; }
S x = { b:3, a:5 };

int[3] a = [ 3, 2, 0 ];
int[3] a = [ 3, 2 ];            // unsupplied initializers are 0, just like in C
int[3] a = [ 2:0, 0:3, 1:2 ];
int[3] a = [ 2:0, 0:3, 2 ];     // if not supplied, the index is the
                                // previous one plus one.

enum color { black, red, green }
int[3] c = [ black:3, green:2, red:5 ];

char[]  file        = `c:\root\file.c`;
char[]  quoteString = \"  r"[^\\]*(\\.[^\\]*)*"  \";

char[]  hello     = "hello world" \n;
char[]  foo_ascii = "hello";       // string is taken to be ascii
wchar[] foo_wchar = "hello";       // string is taken to be wchar

enum COLORS { red, blue, green };

char[][COLORS.max + 1] cstring = [
    COLORS.red   : "red",
    COLORS.blue  : "blue",
    COLORS.green : "green",
];

const ushort table1[16] = [
    0x00A7, 0x0322, 0x07AD, 0x0428,
    0x0536, 0x06B3, 0x023C, 0x01B9
];

const ushort table2[16] = [ 0x0000, 0x0385, 0x070A, 0x048F,
                            0x0536, 0x06B3, 0x023C, 0x01B9];

