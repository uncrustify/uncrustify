// Test: align_func_proto_span_num_cmt_lines - Comment lines between function prototypes

namespace TestNamespace {
VeryLongReturnType function_one();
// Single comment line
int                function_two(int x);/* this is
	                                * a multi-line C comment */
// With a C++ Comment line, forming 2 comment lines
short              function_three();
// Comment line 1
// Comment line 2
// Comment line 3
unsigned int function_four(const char* str);     /* this is
	                                          * a multi-
	                                          * line C comment */
int          function_five(const char* str);
// Comment line 1
// Comment line 2
double       function_six();

char last_function_seven();
}
