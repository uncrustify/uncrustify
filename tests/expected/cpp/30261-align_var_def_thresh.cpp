void testShortTypes()
{
// No stars
	float  a;
	double b;

// All stars
	float&  a;
	double& b;

	float *  a;
	double * b;

	float & a;
	double &b;

// One star before
	double& a;
	float   b;

	double & a;
	float    b;

	double &a;
	float   b;

// One star after
	float   b;
	double& a;

	float    b;
	double & a;

	float   b;
	double &a;
}

void testLongTypes()
{
	int      int_var;
	int *    int_ptr_var;
	int *    int_ptr_var;
	float    float_var;
	float &  float_ref_var;
	float &  float_ref_var;
	double & double_var;
	SomeLongNamespace::SomeLongType long_var;
	int * other_int_var;
	SomeLooooongType long_var;
	SomeLoooooooooongType looong_var;
	int int_var;
	SomeLongNamespace::OtherLongNamespace::SomeLongType very_long_var;
	int *    int_ptr_var;
	float    float_var;
	float &  float_ref_var;
	double & double_var;
	SomeLongNamespace::SomeLongType long_var;
	float float_var;
	int * other_int_var;
	int   other_int_var;
	int * other_int_var;
	int&  other_int_var;
}
