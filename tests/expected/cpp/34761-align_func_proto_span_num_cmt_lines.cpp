// Test: align_func_proto_span_num_cmt_lines - Comment lines between function prototypes

namespace TestNamespace {
VeryLongReturnType function_one();
// Single comment line
int                function_two(int x);
// Comment line 1
// Comment line 2
short function_three();
// Comment line 1
// Comment line 2
// Comment line 3
unsigned int function_four(const char* str);

double last_function_five();
}
