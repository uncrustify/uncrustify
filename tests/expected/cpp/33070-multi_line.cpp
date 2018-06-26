
void func_a ( int a, string b, char c );

void func_b ( int a,
              string b, char c );

void func_c ( int a, string b, char c
              );

void func_d ( int aaaaaaaaaaaaaa, string bbbbbbbbbbbbbb,
              char cccccccccccccccccc );

void func_a ( int a, string b, char c )
{
	return;
}

void func_b ( int a,
              string b, char c )
{
	return;
}

void func_c ( int a, string b, char c
              )
{
	return;
}

void func_d ( int aaaaaaaaaaaaaa, string bbbbbbbbbbbbbb,
              char cccccccccccccccccc )
{
	return;
}

void func_call()
{
	func_a ( 1, 2, 3);
	func_b ( 4,
	         5, 6 );
	func_c ( 7, 8, 9
	         );

	func_d ( "aaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbb",
	         "cccccccccccccccccccccc" );
}
