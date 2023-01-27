void
foo(Foo const & f)
{
    auto bar = Bar(f);
    if (bar.something) {
        assert(bar.something_else == 1);
    }
}

