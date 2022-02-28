void foo(a, b)
{
    bool called = false;

    auto callback = [&](float value)
    {
        called = true;
    };
}
