int main (int argc, const char * argv[])
{
	switch (argc)
	{
	case 0 ... 1:
		return 1;
	}
	return 0;
}