/* Test: align_func_params_span with mixed empty/comment/PP lines */

void test_function(VeryLongParameterType param1,
                   int param2,
#ifdef FEATURE_A
                   /* Platform specific */
                   double param3,

                   /* Another comment */
                   short param4,
#endif
#ifdef FEATURE_B
                   /* Multiple lines here */
                   long int param5,
                   /* Comment line 1 */
                   /* Comment line 2 */
                   /* Comment line 3 */
                   unsigned long param6,


                   /* Two empty + comment */
                   char param7,
#endif

                   float param8);

void another_function(VeryLongParameterType param1,


                       int param2);

void third_function(VeryLongParameterType param1,
#if 1
#endif
#if 1
#endif
                    int param2);

void fourth_function(VeryLongParameterType param1,
                     /* cmt 1 */
                     /* cmt 2 */
                     /* cmt 3 */
                     /* cmt 4 */
                     int param2);
