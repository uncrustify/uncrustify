// Test: align_pp_define_span_num_mixed
// Mixed empty, comment, and preprocessor lines between calls

/* Test input for align_pp_define_span budget options:
 *   align_pp_define_span_num_empty_lines
 *   align_pp_define_span_num_cmt_lines
 *   align_pp_define_span_num_pp_lines
 *
 * Each section is designed so that:
 *   - budget = 0 (no limit): all defines in the section align together
 *   - budget = 1: the separator exceeds the budget, splitting into two groups
 */

/* ------------------------------------------------------------------ */
/* Section A: #define values macros ('as' stack)                      */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Section A-1: empty lines between #define groups                    */
/* ------------------------------------------------------------------ */

#define SHORT_A   1
#define MEDIUM_AB 2

#define LONG_ABCDE 3


#define SHORT_B 10



#define MEDIUM_BB 20
#define LONG_BBC  30

/* ------------------------------------------------------------------ */
/* Section A-2: comment lines between #define groups                  */
/* ------------------------------------------------------------------ */

#define SHORT_A   1
#define MEDIUM_AB 2
// COMMENT 1
#define LONG_ABCDE 3
// COMMENT 1
// COMMENT 2
#define SHORT_B 10
// COMMENT 1
// COMMENT 2
// COMMENT 3
#define MEDIUM_BB 20
#define LONG_BBC  30

/* ------------------------------------------------------------------ */
/* Section A-3: non-#define preprocessor lines between #define groups */
/* ------------------------------------------------------------------ */

#define SHORT_A    1
#define MEDIUM_AB  2
#ifdef A
#define LONG_ABCDE 3
#endif
#ifdef B
#define SHORT_B    10
#endif
#ifdef C
#ifdef D
#define MEDIUM_BB  20
#define LONG_BBC   30
#endif
#endif

/* ------------------------------------------------------------------ */
/* Section A-4: Mixed type of lines between #define groups */
/* ------------------------------------------------------------------ */

#define SHORT_A   1
#define MEDIUM_AB 2
#ifdef A

#define LONG_ABCDE 3
// comment 1
#endif

#define SHORT_B 10

#ifdef C
#ifdef D
// comment 1
// comment 2
#define MEDIUM_BB 20
#define LONG_BBC  30
#endif
#endif

/* ------------------------------------------------------------------ */
/* Section B: function-like macros (use 'asf' stack, not 'as' stack)  */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Section B-1: empty lines between #define groups                    */
/* ------------------------------------------------------------------ */

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(x, lo, hi) (MAX(lo, MIN(x, hi)))


#define ABS(x) ((x) < 0 ? -(x) : (x))



#define ABS2(x) ((x) < 0 ? -(x) : (x))

/* ------------------------------------------------------------------ */
/* Section B-2: comment lines between #define groups                  */
/* ------------------------------------------------------------------ */

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
// comment 1
#define CLAMP(x, lo, hi) (MAX(lo, MIN(x, hi)))
// comment 1
// comment 2
#define ABS(x) ((x) < 0 ? -(x) : (x))
// comment 1
// comment 2
// comment 3
#define ABS2(x) ((x) < 0 ? -(x) : (x))

/* ------------------------------------------------------------------ */
/* Section B-3: non-#define preprocessor lines between #define groups */
/* ------------------------------------------------------------------ */
#define MAX(a, b)        ((a) > (b) ? (a) : (b))
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#ifdef A
#define CLAMP(x, lo, hi) (MAX(lo, MIN(x, hi)))
#endif
#ifdef B
#define ABS(x)           ((x) < 0 ? -(x) : (x))
#endif
#ifdef C
#ifdef D
#define ABS2(x)          ((x) < 0 ? -(x) : (x))
#endif
#endif

/* ------------------------------------------------------------------ */
/* Section A-4: Mixed type of lines between #define groups */
/* ------------------------------------------------------------------ */
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#ifdef A

#define CLAMP(x, lo, hi) (MAX(lo, MIN(x, hi)))
#endif
#ifdef B
// comment 1

#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

// comment 1
#ifdef C
// comment 2
#ifdef D
#define ABS2(x) ((x) < 0 ? -(x) : (x))
#endif
#endif
