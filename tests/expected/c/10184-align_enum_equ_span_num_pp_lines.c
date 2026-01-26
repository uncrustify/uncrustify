/* Test: align_enum_equ_span_num_pp_lines - PP lines between enum values */

enum TestEnum
{
    VERY_LONG_VALUE_NAME = 100,
#ifdef FEATURE_A
    SHORT_VAL            = 200,
#endif
#ifdef FEATURE_B
    MEDIUM_VALUE = 300,
#endif
#if defined(FEATURE_C)
#define MACRO1
    ANOTHER_LONG_NAME = 400,
#endif

    FINAL_VAL = 500
};
