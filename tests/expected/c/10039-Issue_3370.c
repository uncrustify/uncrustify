int main(int argc, char** argv)
{
	switch (argc)
	{
	// If FOO or BAR is defined, treat it specially
	#ifdef FOO
		case FOO:
			return 1;
	#endif
	#ifdef BAR
		case BAR:
			return 2;
	#endif
	default:
		return 100;
	}
}
