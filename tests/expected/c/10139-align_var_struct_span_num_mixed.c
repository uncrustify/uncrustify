// Test: align_var_struct_span with mixed empty/comment/PP lines
// Tests interleaved PP, comments, and empty lines

struct TestStruct
{
	int               x;
	unsigned long int y;
#ifdef A
	//!< 1 PP + 1 cmt between y and z
	double            z;

	//!< 1 empty + 1 cmt between z and w
	float             w;
#endif
#ifdef B
	//!< 2 PP + 1 cmt between w and s
	long s;
	//!< 1 cmt between s and t
	//!< 2 cmt
	//!< 3 cmt
	long int t;


	//!< 2 empty + 1 cmt between t and u
	char u;
#endif
};
