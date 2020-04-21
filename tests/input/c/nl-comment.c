/**
 * This is your typical header comment
 */
 int foo(int bar)
 {
    int idx;
    int res = 0;     // trailing comment
                   // that spans two lines
    /* multi-line comment
    */
    idx = 50;
    /* single line comment */
    for (idx = 1; idx < bar; idx++)
       /* comment in virtual braces */
       res += idx;
    switch (res)
    {
        case 1:
            // C++-style comment
            res++;
            break;
        case 2:
            /* C-style comment */
            res--;
            break;
        case 3:
            /* Multi-line comment
            */
            res = 0;
            break;
        case 4:

            // C++-style comment
            res++;
            break;
        case 5:

            /* C-style comment */
            res--;
            break;
        default:

            /* Multi-line comment
            */
            res = 0;
            break;
    }

    res *= idx;       // some comment

                      // almost continued, but a NL in between

    i++;
// col1 comment in level 1
// second comment
   return(res);
}

   // col1 comment in level 0
 // and another
