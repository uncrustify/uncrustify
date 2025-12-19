// Test: align_typedef_span_num_pp_lines - PP lines between typedefs

typedef VeryLongTypeName TypeAlias1;
#ifdef FEATURE_A
typedef int              TypeAlias2;
#endif
#ifdef FEATURE_B
typedef double TypeAlias3;
#endif
#if defined(FEATURE_C)
#define MACRO1
typedef unsigned char TypeAlias4;
#endif

typedef long int TypeAlias5;
