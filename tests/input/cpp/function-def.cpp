int &Function()
{
static int x;
return (x);
}

void foo1(int param1, int param2, char *param2);

void foo2(int param1,
          int param2,
          char *param2);

void foo3(int param1,
          int param2,                                        // comment
          char *param2
          );

struct whoopee *foo4(int param1, int param2, char *param2    /* comment */);

const struct snickers *
foo5(int param1, int param2, char *param2);


void foo(int param1, int param2, char *param2)
{
   printf("boo!\n");
}

int classname::method();

int classname::method()
{
   foo();
}

int
classname::method2();

int
classname::method2()
{
   foo2();
}

const int& className::method1(void) const
{
    // stuff
}

const longtypename& className::method2(void) const
{
    // stuff
}

int &foo();

int &foo()
{
   list_for_each(a,b) {
      bar(a);
   }
   return nuts;
}

void Foo::bar() {}

Foo::Foo() {}

Foo::~Foo() {}

void func(void)
{
Directory dir("arg");
}
