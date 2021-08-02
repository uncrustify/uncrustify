void f(int i)
{
	switch(i)
	{
	case 1 ... 2:
	{
		break;
	}
	case 3 ...       5:
		break;

	default:
		break
	}
}
