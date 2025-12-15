// Test: align_func_proto_span with mixed empty/comment/PP lines

class ComplexAPI {
public:
VeryLongReturnType complex_func_one();
unsigned int       complex_func_two();
#ifdef PLATFORM_A
// Platform specific function
double             complex_func_three();

// Another comment
short              complex_func_four();
#endif
#ifdef PLATFORM_B
// Multiple lines here
long int           complex_func_five();
// Comment line 1
// Comment line 2
// Comment line 3
unsigned long      complex_func_six();


// Two empty + comment
int                complex_func_seven();
#endif

void               final_func_eight();
};
