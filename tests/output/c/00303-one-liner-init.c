enum boo {
   FOO = 1
};

void foo(void)
{
   char str[123] =
   {
      0
   };

   enum hoo {
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
