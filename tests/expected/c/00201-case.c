void foo(void)
{
	switch(ch)
	{
	// handle 'a'
	case 'a':
	{
		handle_a();
		multiline(123,
		          345);
		break;
	}

	// handle 'b'
	case 'b':
		handle_b();
		multiline(123,
		          345);
		break;

	// handle 'c' and 'd'
	case 'c':
	case 'd':
		// c and d are really the same thing
		handle_cd();
		multiline(123,
		          345);
		break;

	case 'e':
	{
		handle_a();
		multiline(123,
		          345);
	}
	break;

	// case1
	case (case1):
	{
		//do stuff
		break;
	}

	case (case2):
	{
		//do stuff
		break;
	}

	case (case3):

		/*do stuff*/
		break;

	case (case3):
		statement();
		{
			another_statement();
		}
		break;

	// really should not get here
	default:
		handle_default();
		multiline(123,
		          345);
		break;
	}
	multiline(123,
	          345);
}

