class SomeClass
{
public:
// Short parameters
TYPE_EXPORT method1(int   a,
                    float b);

TYPE_EXPORT method2(int&  d,
                    float e);

TYPE_EXPORT method3(int*  f,
                    float g);

// Parameters with '&' and '*'
TYPE_EXPORT method4(int a);
TYPE_EXPORT method5(int & a);
TYPE_EXPORT method6(int * a);

TYPE_EXPORT method7(float a);
TYPE_EXPORT method8(float & a);
TYPE_EXPORT method9(float * a);

// Single short and long parameters
void method10(int a);
void method11(float & a);
void method12(SomeLongNamespace::SomeLongType long_parameter_name);
void method13(double * a);
void method14(SomeLongType long_parameter_name);

// Long parameters
void method20(int * int_param,
              SomeLongNamespace::SomeLongType long_parameter_name,
              float & float_param);

// Possible bug: different aligning in method21 and method22
// align_func_params_span = 1, align_func_params_thresh = 8
void method21(SomeLoooooooooooooongType long_param_1,
              const string&    string_param_1,
              const TimePoint& time_param,
              double double_param_1,
              double double_param_2,
              const string& string_param_2,
              SomeLoooooooooooooongType long_param_2 );
void method22(SomeLoooooooooooooongType long_param_1,
              const string& string_param_1,
              double double_param_1,
              double double_param_2,
              const TimePoint& time_param,
              const string&    string_param_2,
              SomeLoooooooooooooongType long_param_2 );

void method23(int     int_param,
              int *   int_ptr_param,
              float   float_param,
              float & float_ref_param,
              SomeLongNamespace::SomeLongType long_parameter_name,
              int * other_int_param,
              SomeLooooongType long_parameter_name,
              SomeLoooooooooongType looong_parameter_name,
              SomeLongNamespace::OtherLongNamespace::SomeLongType very_long_parameter_name,
              int *    int_ptr_param,
              float    float_param,
              float &  float_ref_param,
              double & double_param,
              SomeLongNamespace::SomeLongType long_parameter_name,
              int * other_int_param);

// Don't align several parameters in one line
void method30(int* f, char foo,
              float g);

// Short parameters in method definition
void method40(int   a,
              float b)
{
	int c;

	if ( true ) callProc;
	// do stuff.
}

// Long parameters in method definition
void method50(int int_param,
              SomeLongNamespace::OtherLongNamespace::SomeLongType long_parameter_name,
              float  float_param,
              double double_param,
              const string & string_param)
{
	doSomething();
}

void method51(
	int int_param,
	SomeLongNamespace::OtherLongNamespace::SomeLongType long_parameter_name,
	float  float_param,
	double double_param,
	const string & string_param)
{
	doSomething();
}
};
