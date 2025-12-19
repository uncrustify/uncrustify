/* Test: align_typedef_span_num_pp_lines - PP lines between typedefs */

typedef unsigned long int ULongInt;
#ifdef FEATURE_A
typedef int               MyInt;
#ifdef FEATURE_B
#define SOMETHING
typedef double            MyDouble;
#ifdef FEATURE_C
#define ANOTHER
#undef OLD
typedef unsigned char     UChar;
#endif
typedef long int          LongInt;
