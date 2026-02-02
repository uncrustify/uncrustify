void my_function()
{
    int outer_var1;
    // Comment for outer_var2
    float outer_var2;

    { // Inner block - this should trigger the recursive call
        double inner_var1;
        #ifdef INNER_DEFINE
        // Inner comment
        char inner_var2;
        #endif
        bool inner_var3;
    }

    long outer_var3;
}
