void foo(int a)
{
   char ch;

   ch = cast(char) a;
   ch = cast(char) 45;
   ch = (char) a; // not a d cast
   ch = (int)45;
   ch = cast(foo)*bar;
   ch = cast(foo)-bar;
   ch = cast(foo)+45;
   ch = cast(foo)&45;
}
