// mod_full_brace_nl_block_rem_mlcond should block brace removal here
if( a == true
    && b == false )
{
	return 1;
}
else if( a == true
         && b == false)
{
	return 2;
}
// except here as there are no parenthesis
else
	return 3;


if( a == true;
    b == true;
    c == true)
{
	return 1;
}

for( a = true;
     a < 9;
     a++)
{
	return 1;
}

while( a == true
       && b == true
       && c == true)
{
	return 1;
}

using (Foo bar =
	       new Foo())
{
	return 1;
}



// mod_full_brace_nl_block_rem_mlcond should not block brace removal here
if( a == true && b == false )
	return 1;
else if( a == true && b == false)
	return 2;
else
	return 3;


if( a == true; b == true; c == true)
	return 1;

for( a = true; a < 9; a++)
{
	return 1;
}

while( a == true && b == true && c == true)
{
	return 1;
}

using (Foo bar = new Foo())
{
	return 1;
}