void Foo()
{
    FooType type
        = isFoo ? fooNamespace::barNamespace::x
            : isBar  ? (isQuux ? fooNamespace::barNamespace::y : fooNamespace::barNamespace::z)
            : isBaz ? (isQuux ? fooNamespace::barNamespace::i : fooNamespace::barNamespace::j)
            : (isQux ? fooNamespace::barNamespace::k : fooNamespace::barNamespace::l);

    fooNamespace::_bar.x
        = FOO_CONSTANT
            | BAR_CONSTANT
            | BAZ_CONSTANT;

    switch (foo)
    {
        case bar:
            FooFunc(x, BarFunc(BazFunc(clamp(x) * 255.0f),
                Round(clamp(g) * 255.0f),
                Round(clamp(b) * 255.0f),
                Round(clamp(a) * 255.0f)), foo);
            return;
    }

    switch (bar)
    {
        case baz:
            RETURN_IF_ERROR(fooNamespace::Foo(x->m_Foo).Bar(
                fooNamespace::fooClien(x->m_Bar), y, x->m_Baz, &z, CONSTANT_FOO), false)
            break;
    }
}
