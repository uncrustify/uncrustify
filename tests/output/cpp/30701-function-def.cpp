void
foo1(
   int param1,
   int param2,
   char *param2
);

void
foo2(
   int param1,
   int param2,
   char *param2
);

void
foo3(
   int param1,
   int param2,                                               // comment
   char *param2
);

struct whoopee *
foo4(
   int param1,
   int param2,
   char *param2                                              /* comment */
);

const struct snickers *
foo5(
   int param1,
   int param2,
   char *param2
);


void
foo(
   int param1,
   int param2,
   char *param2
)
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

