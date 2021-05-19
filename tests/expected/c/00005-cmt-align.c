#ifndef HAVE_FOO
void foo(void)
{
   if (bar)
   {
      call_some_function(); /* call the function */
      return(foo);          /* comment */
   }                        /* if (bar) */
}
#endif /* HAVE_FOO */
#ifndef HAVE_BAR /* bar isn't available on all HW */
void bar(void)
{
   if (foo)
   {
      call_some_function(); /* call the function */
      return(foo);          /* comment */
   } /* if (foo) */
}
#endif /* HAVE_BAR */
