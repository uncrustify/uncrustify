int foo1(int arg)
{
    if (arg == 0) return 1;
    if (arg == 1) /* comment */ return 2;
    if (arg == 2)
        /* C-style comment */
        return 3;
    if (arg == 3) /* trailing comment */
        return 4;
    if (arg == 4) /* trailing comment */
        /* C-style comment */
        return 5;
    if (arg == 5)
        // C++-style comment
        return 6;
    if (arg == 6)
        // Multi-line
        // C++-style comment
        return 7;
    if (arg == 7) // trailing comment
        // Multi-line
        // C++-style comment
        return 8;
    if (arg == 8)
        /*
         * Multi-line C-style comment
         */
        return 9;
    if (arg == 9)
        /*--------------------*/
        /* Multi-part comment */
        /*--------------------*/
        return 10;
    if (arg == 10)
        //-----------------------
        /*
         * Mixed-style comment
         */
        //-----------------------
        return 11;
    if (arg == 11)
        /* comment */ return 12;
    if (arg == 12)

        // C++-style comment
        return 13;
    return arg + 1;
}

int foo2(int arg)
{
    if (arg == 0) { return 1; }
    if (arg == 1) { /* comment */ return 2; }
    if (arg == 2) {
        /* C-style comment */
        return 3;
    }
    if (arg == 3) { /* trailing comment */
        return 4;
    }
    if (arg == 4) { /* trailing comment */
        /* C-style comment */
        return 5;
    }
    if (arg == 5) {
        // C++-style comment
        return 6;
    }
    if (arg == 6) {
        // Multi-line
        // C++-style comment
        return 7;
    }
    if (arg == 7) { // trailing comment
        // Multi-line
        // C++-style comment
        return 8;
    }
    if (arg == 8) {
        /*
         * Multi-line C-style comment
         */
        return 9;
    }
    if (arg == 9)
    {
        /*--------------------*/
        /* Multi-part comment */
        /*--------------------*/
        return 10;
    }
    if (arg == 10)
    {
        //-----------------------
        /* Mixed-style comment */
        //-----------------------
        return 11;
    }
    if (arg == 11)
    {
        /* comment */ return 12;
    }
    if (arg == 12) {

        /* C-style comment */
        return 13;
    }
    return arg + 1;
}

int foo2(int arg)
{
    if (arg == 0) { int a = 1; return a; }
    if (arg == 1) { int a = 2; /* comment */ return a; }
    if (arg == 2) {
        int a = 3;
        /* C-style comment */
        return a;
    }
    if (arg == 3) {
        int a = 4; /* trailing comment */
        return a;
    }
    if (arg == 4) {
        int a = 5; /* trailing comment */
        /* C-style comment */
        return a;
    }
    if (arg == 5) {
        int a = 6;
        // C++-style comment
        return a;
    }
    if (arg == 6) {
        int a = 7;
        // Multi-line
        // C++-style comment
        return a;
    }
    if (arg == 7) {
        int a = 8; // trailing comment
        // Multi-line
        // C++-style comment
        return a;
    }
    if (arg == 8) {
        int a = 9;
        /*--------------------*/
        /* Multi-part comment */
        /*--------------------*/
        return a;
    }
    if (arg == 9) {
        int a = 10;
        /*---------------------*/
        // Mixed-style comment
        /*---------------------*/
        return a;
    }
    if (arg == 11)
    {
        int a = 12;
        /* comment */ return a;
    }
    if (arg == 12) {
        int a = 13;
        /*
         * Multi-line C-style comment
         */
        return a;
    }
    return arg + 1;
}
