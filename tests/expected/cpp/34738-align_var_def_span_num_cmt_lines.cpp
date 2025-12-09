// Test: align_var_def_span_num_cmt_lines in C++ methods
// With span=1 and varying num_cmt_lines settings.

class TestClass
{
public:
void test_method()
{
	int               x; //!< first var
	unsigned long int y; //!< second var
	//!  continued on next line
	double            z; //!< third var
	//!  continued
	//!  on two more lines
	float             w; //!< fourth var
	//!  continued
	//!  on three
	//!  more lines
	long s; //!< fifth var
}
};
