void foo(void)
{
	switch(ch)
	{
		case 'a':
		{
			handle_a();
			multiline(123,
			          345);
			break;
		}

		case 'b':
			handle_b();
			multiline(123,
			          345);
			break;

		case 'c':
		case 'd':
			handle_cd();
			multiline(123,
			          345);
			break;

		case 'e':
		{
			handle_a();
			multiline(123,
			          345);
			break;
		}

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

		default:
			handle_default();
			multiline(123,
			          345);
			break;
	}
	multiline(123,
	          345);
}

