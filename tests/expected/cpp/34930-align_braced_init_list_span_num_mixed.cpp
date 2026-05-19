class TestEmptyLines
{
int a_one   { 1 };
long bc_two { 2 };

unsigned int cd_three { 3 };


short ef_four { 4 };



double gh_five { 5 };
};

class TestCommentLines
{
int a_one { 1 };
// comment line
long bc_two { 2 };
// comment line A
/* comment line B */
unsigned int cd_three { 3 };
// comment line A
// comment line B
// comment line C
short ef_four { 4 };
};

class TestPPLines
{
int a_one             { 1 };
#ifdef FEATURE
long bc_two           { 2 };
#endif
#ifdef FEATURE2
unsigned int cd_three { 3 };
#endif
#ifdef FEATURE3
#ifdef FEATURE4
short ef_four         { 4 };
#endif
#endif
};

class TestMixedLines
{
int a_one { 1 };

#ifdef FEATURE
long bc_two { 2 };
#endif
// comment line A
// comment line B
unsigned int cd_three { 3 };

#ifdef FEATURE2
// comment line C
short ef_four { 4 };
#endif
};

void test_func_empty(void)
{
	int a_one   { 1 };
	long bc_two { 2 };

	unsigned int cd_three { 3 };


	short ef_four { 4 };



	double gh_five { 5 };
}

void test_func_comment(void)
{
	int a_one { 1 };
	// comment line
	long bc_two { 2 };
	// comment line A
	/* comment line B */
	unsigned int cd_three { 3 };
	// comment line A
	// comment line B
	// comment line C
	short ef_four { 4 };
}

void test_func_pp(void)
{
	int a_one             { 1 };
#ifdef FEATURE
	long bc_two           { 2 };
#endif
#ifdef FEATURE2
	unsigned int cd_three { 3 };
#endif
#ifdef FEATURE3
#ifdef FEATURE4
	short ef_four         { 4 };
#endif
#endif
}

void test_func_mixed(void)
{
	int a_one { 1 };

#ifdef FEATURE
	long bc_two { 2 };
#endif
	// comment line A
	// comment line B
	unsigned int cd_three { 3 };

#ifdef FEATURE2
	// comment line C
	short ef_four { 4 };
#endif
}
