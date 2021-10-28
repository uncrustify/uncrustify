int main(int argc, char** argv)
{
	switch (argc)
	{
	case 1:
		// return the number unchanged
		return 1;
	case 2:
	// fall through
	case 3:
		return 5;
	default:
		return 10;
	}
}
