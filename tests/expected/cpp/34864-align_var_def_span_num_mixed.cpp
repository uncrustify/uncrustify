// Test: align_var_def_span with mixed empty/comment/PP lines in C++ methods
// Tests interleaved PP, comments, and empty lines

class TestClass
{
public:
void test_method()
{
	int               x;
	unsigned long int y;
#ifdef A
	//!< 1 PP + 1 cmt between y and z
	double z;

	//!< 1 empty + 1 cmt between z and w
	float w;
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
}
};

class SecondClass
{
public:
void second_method()
{
	char v;


	int ww;
}

void third_method()
{
	char aa;
#if 1
#if 1
#if 1
	int bbb;
#endif
#endif
#endif
}

void fourth_method()
{
	char cc;
	// cmt 1
	// cmt 2
	// cmt 3
	// cmt 4
	int ddd;
}
};
