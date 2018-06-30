// No extra line added
void test1()
{
	if ( i == 10 )
		i++;
}

// No extra line added
void test2()
{
	if ( i == 10 )
	{
		i++;
	}
}

// No extra line added
void test3()
{
	if ( i == 10 )
	{
		if ( j == 10 )
		{
			i++;
		}
	}
}

// No extra line added
void test4()
{
	if ( i == 10 )
	{
		if ( j == 10 )
			i++;
	}
}

// Extra line added (after Uncrustify)
void test5()
{
	if ( i == 10 )
		if ( j == 10 )
		{
			i++;
		}
}

// Extra line added (after Uncrustify)
void test6()
{
	if ( i == 10 )
		if ( j == 10 )
			i++;
}
