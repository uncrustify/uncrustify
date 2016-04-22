#define FOO_MAX 10

bool foo[ FOO_MAX ];

void
foo_bar( int  a,
         int* b,
         bool foo[ FOO_MAX ] );

void
A()
{
    int  a;
    int* b;
    foo_bar( a, b, foo );
}
