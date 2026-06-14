/* Test file for align_right_cmt_span budget options.*/

/* ============================================================
 * Section 1: C-style (/* ... */) trailing comments
* ============================================================ */

void test_c_cmt_empty_lines(void)
{
	int a = 1;   /* comment a */
	int bb = 2;  /* comment bb */

	int ccc = 3; /* comment ccc */


	int dddd = 4; /* comment dddd */



	int eeeee_long = 5; /* comment eeeee_long */
}

void test_c_mlcmt_empty_lines(void)
{
	int a = 1;   /* comment a
	                on multiple lines */
	int bb = 2;  /* comment bb
	                on multiple lines */

	int ccc = 3; /* comment ccc
	                on multiple lines */


	int dddd = 4; /* comment dddd
	                 on multiple lines
	                 and lines */



	int eeeee_long = 5; /* comment eeeee_long */
}

void test_c_cmt_pp_lines(void)
{
	int a = 1;  /* comment a */
	int bb = 2; /* comment bb */
#define PP1 1
	int ccc = 3; /* comment ccc */
#define PP2 2
#define PP22 22
	int dddd = 4; /* comment dddd */
#define PP3 3
#define PP33 33
#define PP333 333
	int eeeee_long = 5; /* comment eeeee_long */
}

void test_c_cmt_comment_lines(void)
{
	int a = 1;  /* comment a */
	int bb = 2; /* comment bb */
	/* standalone C comment 1 */
	int ccc = 3; /* comment ccc */
	/* standalone C comment A */
	/* standalone C comment B */
	int dddd = 4; /* comment dddd */
	/* standalone C comment X */
	/* standalone C comment Y */
	/* standalone C comment Z */
	int eeeee_long = 5; /* comment eeeee_long */
}

class DocumentedClass
{
private:
int a {1};        //!< comment a
int bb;           //!< comment bb

int ccc {3};      //!< comment ccc
                  //!< this is a long explanation
                  //!< of the attribute in doxygen
                  //!< format that many people use...

int ccc_long {3}; //!< comment ccc_long
//!< this is a long explanation starting at same column
//!< of the attribute, in doxygen
//!< format that many people use...


int dddd; //!< comment dddd



int eeeee_long {5}; //!< comment eeeee_long
}

/* ============================================================
 * Section 2: C++-style (//) trailing comments
 * ============================================================ */

void test_cpp_cmt_empty_lines(void)
{
	int a = 1;   // comment a
	int bb = 2;  // comment bb

	int ccc = 3; // comment ccc


	int dddd = 4; // comment dddd



	int eeeee_long = 5; // comment eeeee_long
}

void test_cpp_cmt_pp_lines(void)
{
	int a = 1;  // comment a
	int bb = 2; // comment bb
#define PP1 1
	int ccc = 3; // comment ccc
#define PP2 2
#define PP22 22
	int dddd = 4; // comment dddd
#define PP3 3
#define PP33 33
#define PP333 333
	int eeeee_long = 5; // comment eeeee_long
}

void test_cpp_cmt_comment_lines(void)
{
	int a = 1;  // comment a
	int bb = 2; // comment bb
	// standalone C++ comment 1
	int ccc = 3; // comment ccc
	// standalone C++ comment A
	// standalone C++ comment B
	int dddd = 4; // comment dddd
	// standalone C++ comment X
	// standalone C++ comment Y
	// standalone C++ comment Z
	int eeeee_long = 5; // comment eeeee_long
}

void test_cpp_cmt_comment_multilines(void)
{
	int a = 1;  // comment a
	int bb = 2; // comment bb
	// ml C++ comment 1
	int ccc = 3; // comment ccc
	// ml C++ comment A
	// ml C++ comment B
	int dddd = 4; // comment dddd
	// ml C++ comment X
	// ml C++ comment Y
	// ml C++ comment Z
	int eeeee_long = 5; // comment eeeee_long
}

void test_cpp_cmt_comment_doxygen_multilines(void)
{
	int a = 1;  //!< comment a
	int bb = 2; //!< comment bb
	//!< ml C++ comment 1
	int ccc = 3; //!< comment ccc
	//!< ml C++ comment A
	//!< lm C++ comment B
	int dddd = 4; //!< comment dddd
	//!< ml C++ comment X
	//!< standalone C++ comment Y
	//!< ml C++ comment Z
	int eeeee_long = 5; // comment eeeee_long
}

/* ============================================================
 * Section 3: Mixed C and C++ trailing comments
 * ============================================================ */

void test_mixed_cmt_empty_lines(void)
{
	int a = 1;   /* comment a */
	int bb = 2;  // comment bb

	int ccc = 3; /* comment ccc */


	int dddd = 4; // comment dddd



	int eeeee_long = 5; /* comment eeeee_long */
}

void test_mixed_cmt_pp_lines(void)
{
	int a = 1;  /* comment a */
	int bb = 2; // comment bb
#define PP1 1
	int ccc = 3; /* comment ccc */
#define PP2 2
#define PP22 22
	int dddd = 4; // comment dddd
#define PP3 3
#define PP33 33
#define PP333 333
	int eeeee_long = 5; /* comment eeeee_long */
}

void test_mixed_cmt_comment_lines(void)
{
	int a = 1;  /* comment a */
	int bb = 2; // comment bb
	/* standalone C comment 1 */
	int ccc = 3; /* comment ccc */
	// standalone C++ comment A
	// standalone C++ comment B
	int dddd = 4; // comment dddd
	/* standalone C comment X */
	// standalone C++ comment Y
	/* standalone C comment Z */
	int eeeee_long = 5; /* comment eeeee_long */
}

void test_cpp_cmt_mixed_lines(void)
{
	int a = 1; // comment a

	/* standalone C comment 1 */
	int bb = 2; /* comment bb */

#define PP1 1
	int ccc = 3; // comment ccc

#define PP1 1
	// standalone C++ comment A
	int dddd = 4; /* comment dddd */
	/* standalone C comment X */
	/* standalone C comment Y */
#define PP1 1

	int eeeee_long = 5; /* comment eeeee_long */
#define PP1 1
	/* standalone C comment . */
	/* standalone C comment .. */
	/* standalone C comment ... */

	int fffffffff_long = 6; /* comment fffffffff_long */


	// standalone C++ comment
	int gggggggggg_long = 7; // comment gggggggggg_long
#define PP1 1
#define PP2 2

	int hhhhhhhhhhh_long = 8; // hhhhhhhhhhh_long
#define PP1 1
#define PP2 2
#define PP3 3



	// standalone C++ comment
	// standalone C++ comment
	// standalone C++ comment
	int iiiiiiiiiiiii_long = 9; // iiiiiiiiiiiii_long
}

