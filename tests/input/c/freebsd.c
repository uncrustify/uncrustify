/* Make the structure name match the typedef. */
typedef struct bar {
	int	level;
} BAR;
typedef int		foo;	/* This is foo. */
typedef const long	baz;	/* This is baz. */


static char	*function(int _arg, const char *_arg2, struct foo *_arg3,
		    struct bar *_arg4);
static void	 usage(void);

/*
 * All major routines should have a comment briefly describing what
 * they do.  The comment before the "main" routine should describe
 * what the program does.
 */
int
main(int argc, char *argv[])
{
	char *ep;
	long num;
	int ch;

	while ((ch = getopt(argc, argv, "abNn:")) != -1)
		switch (ch) {		/* Indent the switch. */
		case 'a':		/* Don't indent the case. */
			aflag = 1;	/* Indent case body one tab. */
			/* FALLTHROUGH */
		case 'b':
			bflag = 1;
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'n':
			num = strtol(optarg, &ep, 10);
			if (num <= 0 || *ep != '\0') {
				warnx("illegal number, -n argument -- %s",
				    optarg);
				usage();
			}
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	for (p = buf; *p != '\0'; ++p)
		;	/* nothing */
	for (;;)
		stmt;
	for (;;) {
		zed = a + really + long + statement + that + needs +
		    two + lines + gets + indented + four + spaces +
		    on + the + second + and + subsequent + lines;
	}
	for (;;) {
		if (cond)
			stmt;
	}
	if (val != NULL)
		val = realloc(val, newsize);

        fcn_call(with, a, really, long, list, of, parameters,
                 that, spans, two, lines);

	for (; cnt < 15; cnt++) {
		stmt1;
		stmt2;
	}

        almod = (pc->IsSingleLineComment() &&
                cpd.settings[UO_indent_relative_single_line_comments].b) ?
            ALMODE_KEEP_REL : ALMODE_KEEP_ABS;

	/* Indentation is an 8 character tab.  Second level indents are four spaces.
	 * If you have to wrap a long statement, put the operator at the end of the
	 * line.
	 */

	while (cnt < 20 && this_variable_name_is_too_long &&
	    ep != NULL)
		zappy = a + really + long + statement + that + needs
		    + two + lines + gets + indented + four + spaces +
		    on + the + second + and + subsequent + lines;

   // Do not add whitespace at the end of a line, and only use tabs followed by
   // spaces to form the indentation.  Do not use more spaces than a tab will
   // produce and do not use spaces in front of tabs.
   //
   // Closing and opening braces go on the same line as the else.  Braces that
   // are not necessary may be left out.

	if (test)
		stmt;
	else if (bar) {
		stmt;
		stmt;
	} else
		stmt;

        // No spaces after function names.  Commas have a space after them.  No spa-
        // ces after `(' or `[' or preceding `]' or `)' characters.

	error = function(a1, a2);
	if (error != 0)
		exit(error);

        // Unary operators do not require spaces, binary operators do.  Do not use
        // parentheses unless they are required for precedence or unless the state-
        // ment is confusing without them.  Remember that other people may confuse
        // easier than you.  Do YOU understand the following?

	a = b->c[0] + ~d == (e || f) || g && h ? i : j >> 1;
	k = !(l & FLAGS);


        // Exits should be 0 on success, or 1 on failure.

	exit(0);	   /*
			    * Avoid obvious comments such as
			    * "Exit 0 on success."
                            */
}

static char *
     function(a1, a2, fl, a4)
	     int a1, a2;     /* Declare ints, too, don't default them. */
	     float fl;	     /* Beware double vs. float prototype differences. */
	     int a4;	     /* List in order declared. */
     {
     }
