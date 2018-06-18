void foo(void)
{
	int a = x ? y
	          : z,
	    b = x ? (y)
	          : (z),
	    c = x ? *y
	          : *z,
	    d = x ? &y
	          : &z;


	if (x ? y
	      : z)
	{
		baz;
	}
	if (x ? (y)
	      : (z))
	{
		baz;
	}
	if (x ? *y
	      : *z)
	{
		baz;
	}
	if (x ? &y
	      : &z)
	{
		baz;
	}
}

