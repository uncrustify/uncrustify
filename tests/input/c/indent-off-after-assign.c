void foo(void)
{
   int a;
   int b =
   veryLongMethodCall(
     arg1,
     longMethodCall(
       arg2,
       methodCall(
         arg3, arg4
       )
     )
   );
   junk(a =
   3);
}

void f()
{
  int x = size_t(1.0) +
    2;
  int y = (size_t(1.0) +
    5);

  int z =
  size_t(1.0)
  + 5
  + size_t(2.0);
}
