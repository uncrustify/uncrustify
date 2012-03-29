void f1()
{
    auto a =
    [=](int *a, Something & b)
    {
        std::cout << "blah: " << *a;
    };
}

void f1a()
{
    std::for_each(a, b,
                  [](Something & b)
                  {
                      std::cout << "blah: " << *a;
                  });
}

void f2()
{
    Invoke(a, b,
           [&one, two](int *a, Something & b)
           {
               std::cout << "blah: " << *a;
           });
}
