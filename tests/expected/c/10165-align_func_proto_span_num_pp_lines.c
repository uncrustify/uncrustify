// Test: align_func_proto_span_num_pp_lines - Preprocessor lines between function prototypes in C

LongReturnType api_function_one(void);
#ifdef FEATURE_A
int            api_function_two(void);
#endif
#ifdef FEATURE_B
short          api_function_three(int param);
#endif
#if defined(FEATURE_C)
#define SOME_MACRO
unsigned int api_function_four(void);
#endif

long int final_function_five(void);
