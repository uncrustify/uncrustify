//          gap ↘ |123456| ↙ gap
static int              *a;
static unsigned int    **b;
static short int        *c;
static long int          d;
static int            ***e;
//          gap ↗ |123456| ↖ gap

//                                gap ↘ |12345678| ↙ gap
int function_with_no_star_parameters (int        parameter)
//                                gap ↗ |12345678| ↖ gap
{
	return 0;
}

//                           gap ↘ |12345678| ↙ gap
int function_with_stars(int                *f,
                        unsigned int      **g,
                        short int          *h,
                        long int            i,
                        int              ***j)
//                           gap ↗ |12345678| ↖ gap
{
//           gap ↘ |123456| ↙ gap
	int              *k;
	unsigned int    **l;
	short int        *m;
	long int          n;
	int            ***o;
//           gap ↗ |123456| ↖ gap
	return 0;
}

//                                            gap ↘ |12345678| ↙ gap
int function_closer_than_16_lines_from_j(int                *p,
                                         unsigned int      **q,
                                         short int          *r,
                                         long int            s,
                                         int              ***t)
//                                            gap ↗ |12345678| ↖ gap
{
//           gap ↘ |123456| ↙ gap
	int              *u;
	unsigned int    **v;
	short int        *w;
	long int          x;
	int            ***y;
//           gap ↗ |123456| ↖ gap

	/* The point of this comment is merely to make the
	 * function longer than 16 lines.
	 *
	 * We're doing this to show that parameter alignment is
	 * per parameter list, and that parameter lists from
	 * multiple functions aren't lined up with each other.
	 *
	 * You can see that by noticing that function_with_stars and this
	 * function don't have parameter lists aligned with each
	 * other, and this function and the function below it are
	 * too far away anyway.
	 */
	return 0;
}

//                                 gap ↘ |12345678| ↙ gap
int more_than_16_lines_from_t(int                *z,
                              unsigned int      **zz,
                              short int          *zzz,
                              long int            zzzz,
                              int              ***zzzzz)
//                                 gap ↗ |12345678| ↖ gap
{
//           gap ↘ |123456| ↙ gap
	int              *zzzzzz;
	unsigned int    **zzzzzzz;
	short int        *zzzzzzzz;
	long int          zzzzzzzzz;
	int            ***zzzzzzzzzz;
//           gap ↗ |123456| ↖ gap
	return 0;
}
