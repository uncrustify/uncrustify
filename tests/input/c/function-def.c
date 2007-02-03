void foo1(int param1, int param2, char *param2);

void foo2(int param1,
          int param2,
          char *param2);

void foo3(int      param1,
          int     param2,
          char  *param2
          );

struct whoopee *foo4(int param1, int param2, char *param2);

const struct snickers *
foo5(int param1, int param2, char *param2);


void foo(int param1, int param2, char *param2)
{
   printf("boo!\n");
}

EXPORT int DoStuff(int    Num);

