// Test: align_func_proto_span with mixed empty/comment/PP lines in C

VeryLongReturnType complex_func_one(void);
unsigned int       complex_func_two(void);
#ifdef PLATFORM_A
// Platform specific function
double             complex_func_three(void);

// Another comment
short              complex_func_four(void);
#endif
#ifdef PLATFORM_B
// Multiple lines here
long int           complex_func_five(void);
// Comment line 1
// Comment line 2
// Comment line 3
unsigned long      complex_func_six(void);


// Two empty + comment
int                complex_func_seven(void);
#endif

void               final_func_eight(void);
