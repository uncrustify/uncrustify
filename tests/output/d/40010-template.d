
template Foo(T, U)
{
    class Bar { }

    T foo(T t, U u)
    {
    }

    T abc;

    typedef T * FooType;
}

alias Foo!(int, char)f;
f.Bar b;
f.foo(1, 2);
f.abc = 3;

class Abc(T)
{
    T t;
}

