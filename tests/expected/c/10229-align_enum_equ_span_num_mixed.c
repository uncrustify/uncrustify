/* Test: align_enum_equ_span with mixed empty/comment/PP lines */

enum TestEnum
{
	VERY_LONG_VALUE_NAME = 100,
	SHORT_VAL            = 200,
#ifdef FEATURE_A
	// Platform specific
	MEDIUM_VALUE         = 300,

	// Another comment
	TINY_VAL             = 400,
#endif
#ifdef FEATURE_B
	// Multiple lines here
	ANOTHER_LONG_NAME    = 500,
	// Comment line 1
	// Comment line 2
	// Comment line 3
	EXTRA_VALUE          = 600,


	// Two empty + comment
	LAST_LONG_VALUE = 700,
#endif

	FINAL_VAL       = 800
};

enum SecondEnum
{
	NEXT_LONG_VALUE = 100,


	SHORT_NEXT = 200
};

enum ThirdEnum
{
	ANOTHER_VALUE_NAME = 100,
#if 1
#if 1
#if 1
	TINY = 200
#endif
#endif
#endif
};

enum FourthEnum
{
	EXTRA_LONG_VALUE_NAME = 100,
	// cmt 1
	// cmt 2
	// cmt 3
	// cmt 4
	SMALL = 200
};
