int main(int argc, char **argv)
{
	int x = argc * argc;
	int y[] = { argc * argc };
	struct { int x; } z = { argc * argc };
	enum { w = 5 * 5 };
	return 0;
}
