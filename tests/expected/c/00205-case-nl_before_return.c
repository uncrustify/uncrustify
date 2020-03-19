int foo(int arg)
{
	switch (arg)
	{
	case 0: return 1;
	case 1:
		return 2;
	case 2:
		printf("Hello world!\n");
		return 3;
	case 3:
	{
		int a = 4;
		return a;
	}
	case 4:

		return 5;
	case 5:
		printf("Hello world!\n");

		return 6;
	case 6:
	{
		int a = 7;

		return a;
	}
	default:
		return arg++;
	}
	return 0;
}
