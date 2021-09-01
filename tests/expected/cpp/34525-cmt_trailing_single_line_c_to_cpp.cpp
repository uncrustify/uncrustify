int main(int argc, char **argv){

	// C-style comments on same line with actual code
	// ----------------------------------------------

	int a = 5; // Trailing, single-line C-style comment

	int b = /* Single-line C-style comment in the middle */ 5;

	/* Single-line C-style comment at beginning of line */ int c =  5;

	int d = 5; /* Trailing
	              Multi-line
	              C-style
	              comment */

# define A_MACRO \
	do { \
		if (true) { \
			int e = 5; /* Trailing single-line C-style comment inside macro*/ \
		} \
	} while (0)


	// C-style comments with no actual code on the same line
	// -----------------------------------------------------

	// Single-line C-style comment.

	/* Multi-line
	 * C-style
	 * comment.
	 * */
}
