/* Test: align_func_params_span_num_pp_lines - PP lines between parameters */

void test_function(VeryLongParameterType param1,
#ifdef FEATURE_A
                   int                   param2,
#endif
#ifdef FEATURE_B
                   double                param3,
#endif
#if defined(FEATURE_C)
#define MACRO1
                   unsigned char param4,
#endif

                   long int param5);
