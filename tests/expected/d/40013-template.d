template Foo(T, U)
{
    class Bar { }

    T foo(T t, U u) {
    }

    T abc;

    typedef T*  FooType;
    typedef Tte**  FooType0;
    typedef int* FooType1;
    typedef const char FooType2;
}

alias Foo!(int, char) f;
f.Bar b;
f.foo(1,2);
f.abc = 3;

to!string(10.0);
TFoo!int.t x;

class Abc(T)
{
T t;
}

