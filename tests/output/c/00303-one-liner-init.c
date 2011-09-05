enum boo
{
   FOO = 1
};

void foo(void)
{
   char str[123] =
   {
      0
   };

   enum hoo
   {
      NOO = 1
   };

   strcat(str, "foo");
}

void f()
{
   if (bar())
      baz(1);
   else
      baz(2);
}

int foo()
{
   return 0;
}

void foo(int a, int b)
{
   if (a == b)
   {
      a++;
   }
   else
   {
      b++;
   }
   if (a == b)
      a++;
   else
      b++;
}
