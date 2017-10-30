void Foo1()
{
    switch (foo)
    {
        case 1:
            _bar = new Bar(x, y,
                z, a);
            break;
        case 2:
            _bar = new Bar(x, y,
            z, a);
            break;
        case 3:
            _bar = foo.bar;
            break;
        case 4:
            foo.bar = Bar.BarFunc(x, (x == y)
                ? foo.x
                : foo.y);
            break;
        case 5:
            foo.bar = Bar.BarFunc(x, (x == y)
            ? foo.x
            : foo.y);
            break;
    }
}
