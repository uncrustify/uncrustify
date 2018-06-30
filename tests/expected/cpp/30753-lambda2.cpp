void f1()
{
   auto a =
      [=] (int *a, Something& b) {
         std::cout << "blah: " << *a;
      };
}

void f1a()
{
   std::for_each(a, b,
                 [] (Something& b) {
                    std::cout << "blah: " << *a;
                 }
                 );
}

void f1b()
{
   std::for_each(a, b,
                 [] (int& b) -> foo {
                    b += 3;
                    return(b);
                 }
                 );
}

void f2()
{
   Invoke(a, b,
          [&one, two] (int *a, Something& b) {
             std::cout << "blah: " << *a;
          }
          );
}

void f3a()
{
   auto a = [] {};
   auto b = [] { return(true); };
}

void f3b()
{
   Invoke(a, b,
          [&one, two] {
             std::cout << "blah: " << one << two;
          }
          );
}

void f3c()
{
   int a[]{};
}

void g1()
{
   auto a = [=] (int *a, Something&b) { std::cout << "blah: " << *a; };
}

void g1a()
{
   std::for_each(a, b, [] (Something& b) { std::cout << "blah: " << *a; });
}

void g1b()
{
   std::for_each(a, b, [] (int& b) -> foo { b += 3; return(b); });
}

void g2()
{
   Invoke(a, b,
          [&one, two] (int *a, Something&b) { std::cout << "blah: " << *a; });
}
