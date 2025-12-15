// Test: align_func_proto_span_num_pp_lines - Preprocessor lines between function prototypes

class APIClass {
public:
LongReturnType api_function_one();
#ifdef FEATURE_A
int            api_function_two();
#endif
#ifdef FEATURE_B
short api_function_three(int param);
#endif
#if defined(FEATURE_C)
#define SOME_MACRO
unsigned int api_function_four();
#endif

long int final_function_five();
};
