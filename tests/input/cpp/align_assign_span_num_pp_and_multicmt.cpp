// Test: align_assign_span_num_pp_and_multicmt - PP directives and multi-line comments

class TestClass
{
public:
    void test_method()
    {
        VeryLongVariableType var1 = 100;
#ifdef FEATURE_A
        int var2 = 200;
#endif
        /* Multi-line comment
           spanning multiple lines
           to test filtering */
        double var3 = 300.0;
#if defined(FEATURE_B)
#define SOME_MACRO
        unsigned char var4 = 400;
#endif
        /* Another
         * multi-line
         * comment */
        long int var5 = 500;
    }

    enum TestEnum
    {
        VERY_LONG_ENUM_VALUE = 1000,
        /* Multi-line
           comment in enum */
        SHORT_ENUM = 2000,
#ifdef PLATFORM_A
        PLATFORM_VAL = 3000,
#endif
        FINAL_VAL = 4000
    };
};
