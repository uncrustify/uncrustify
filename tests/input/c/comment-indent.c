/**
 * This is your typical header comment
 */
 int foo(int bar)
 {
    int idx;
    int res = 0;     // trailing comment
                   // that spans two lines
    for (idx = 1; idx < bar; idx++)
       /* comment in virtual braces */
       res += idx;

    res *= idx;       // some comment

                      // almost continued, but a NL in between

// col1 comment in level 1
// second comment
   return(res);
}

   // col1 comment in level 0
 // and another

void foo()
{
 if( bar )
 {
   foo();
 }
 /*else if( bar2 )
 {
   foo2();
 }
 */else if( bar3 )
 {
   foo3();
 }
}

