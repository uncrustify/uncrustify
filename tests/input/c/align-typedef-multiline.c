// AlignStack::Flush() tightens the gap to 1 for a lone typedef, but only when
// the 'typedef' keyword sits on the same line as the aligned name. These
// multiline forms put it on an earlier line and must not be tightened.

typedef struct foo {
	int a;
	char *b;
} bar;

typedef struct {
	int x;
	double yy;
} anon_t;

typedef enum {
	A,
	BB,
} letters_t;

typedef
struct baz
split_t;

// A multiline typedef directly above a single line one: the backwards scan for
// 'typedef' must stop at the start of the line and not reach into the form
// above.

typedef struct wide {
	int w;
} wide_t;
typedef int after_multiline_t;

// A lone single line typedef does have 'typedef' on its own line, so this one
// is tightened to a gap of 1 rather than align_typedef_gap.

struct sep1 { int a; };

typedef int LONE_T;

struct sep2 { int b; };

// Two adjacent typedefs align against each other instead.

typedef int PAIR_A;
typedef unsigned long PAIR_B;

// A typedef nested one level down, so the level check matters too.

struct outer {
	typedef struct inner {
		int q;
	} inner_t;
	typedef int nested_lone_t;
};
