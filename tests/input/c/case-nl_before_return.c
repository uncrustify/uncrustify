int foo(int arg)
{
    switch (arg)
    {
        case 0: return 1;
        case 1:
            return 2;
        case 2:
            printf("Hello world!\n");
            return 3;
        case 3:
        {
            int a = 4;
            return a;
        }
        case 4:

            return 5;
        case 5:
            printf("Hello world!\n");

            return 6;
        case 6:
        {
            int a = 7;

            return a;
        }
        case 7: /* comment */ return 8;
        case 8:
            /* C-style comment */
            return 9;
        case 9: /* trailing comment */
            return 10;
        case 10: /* trailing comment */
            /* C-style comment */
            return 11;
        case 11:
            // C++-style comment
            return 12;
        case 12:
            // Multi-line
            // C++-style comment
            return 13;
        case 13: // trailing comment
            // Multi-line
            // C++-style comment
            return 14;
        case 14:

            // Multi-line
            // C++-style comment
            return 15;
        case 15:

            /* C-style comment */
            return 16;
        case 16:
            /*
             * Multi-line C-style comment
             */
            return 17;
        case 17:
            /*--------------------*/
            /* Multi-part comment */
            /*--------------------*/
            return 18;
        case 18:
            /*---------------------*/
            // Mixed-style comment
            /*---------------------*/
            return 19;
        default:
            return arg++;
    }
    return 0;
}
