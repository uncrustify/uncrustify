// Test: align_typedef_span with mixed empty/comment/PP lines

typedef unsigned long int ULongInt;
typedef int               MyInt;
#ifdef FEATURE_A
// Platform specific
typedef double MyDouble;

// Another comment
typedef short MyShort;
#endif
#ifdef FEATURE_B
// Multiple lines here
typedef long int LongInt;
// Comment line 1
// Comment line 2
// Comment line 3
typedef unsigned char UChar;


// Two empty + comment
typedef char MyChar;
#endif

typedef float MyFloat;

typedef unsigned long int AnotherLong;


typedef int ShortT;

typedef unsigned long int ExtraLongName;
#if 1
#if 1
#if 1
typedef int TinyT;
#endif
#endif
#endif

typedef unsigned long int VeryLongTypeName;
// cmt 1
// cmt 2
// cmt 3
// cmt 4
typedef int SmT;
