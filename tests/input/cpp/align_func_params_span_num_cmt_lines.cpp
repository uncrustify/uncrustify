// Test: align_func_params_span_num_cmt_lines - Comment lines between parameters

void test_function(VeryLongParameterType param1,
                   // Single comment
                   int param2,
                   // Comment 1
                   // Comment 2
                   double param3,
                   // Comment 1
                   // Comment 2
                   // Comment 3
                   unsigned char param4,

                   long int param5);
