/* Test: align_assign_span_num_pp_lines - PP lines between assignments */

void test_function()
{
    VeryLongVariableType var1 = 100;
#ifdef FEATURE_A
    int var2                  = 200;
#endif
#ifdef FEATURE_B
    double var3               = 300.0;
#endif
#if defined(FEATURE_C)
#define MACRO1
    unsigned char var4        = 400;
#endif

    long int var5 = 500;
}
