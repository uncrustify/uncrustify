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
long int complex_func_five();
// Comment line 1
// Comment line 2
// Comment line 3
unsigned long complex_func_six();


// Two empty + comment
int  complex_func_seven();
#endif

void final_func_eight();
};

class AnotherAPI
{
public:
VeryLongReturnType another_func_nine();


int                short_ten();

VeryLongReturnType long_named_func_eleven();
#if 1
#if 1
#if 1
int twelve();
#endif
#endif
#endif

VeryLongReturnType extra_long_func_thirteen();
// cmt 1
// cmt 2
// cmt 3
// cmt 4
int fn_fourteen();
};
