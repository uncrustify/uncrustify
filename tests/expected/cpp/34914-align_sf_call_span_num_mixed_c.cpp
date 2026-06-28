/* This file has 2 copies that should remain the same:
 *   - input/c/align_sf_call_span_num_mixed.c
 *   - input/cpp/align_sf_call_span_num_mixed_c.cpp
 * It is a copy instead of a file in a 'common' directory so
 * it keeps expected outputs into separate directories.
 * If you modify this file, please modify the other.
 * (See https://github.com/uncrustify/uncrustify/pull/4714)
 */
void test_empty_lines(void)
{
	align_prams(param1 + param1, param2, param3);

	align_prams(p41, p5, p6);


	align_prams(p711, p8, p9);



	align_prams(p7111, p8, p9);
}

void test_comment_lines(void)
{
	align_prams(param1 + param1, param2, param3);
	/* comment line */
	align_prams(p41,             p5,     p6);
	/* comment line A */
	/* comment line B */
	align_prams(p711,            p8,     p9);
	/* comment line A
	   is multi-line and count for two */
	align_prams(p71133,          p8,     p9);
	/* comment line A */
	/* comment line B */
	/* comment line C */
	align_prams(p7222, p8, p9);
}

void test_pp_lines(void)
{
	align_prams(param1 + param1, param2, param3);
#ifdef FEATURE
	align_prams(p41, p5, p6);
#endif
#ifdef FEATURE2
	align_prams(p711, p8, p9);
#endif
#ifdef FEATURE3
#ifdef FEATURE4
	align_prams(p7222, p8, p9);
#endif
#endif
}

void test_mixed_lines(void)
{
	align_prams(param1 + param1, param2, param3);

#ifdef FEATURE
	align_prams(p41, p5, p6);
#endif
	/* comment line A */
	/* comment line B */

	align_prams(p711, p8, p9);

#ifdef FEATURE2
	/* comment line C */
	align_prams(p7222, p8, p9);
#endif
}
