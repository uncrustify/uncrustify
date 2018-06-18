
int foo(int bar)
{
	for (;;)
	{
		break;
	}
	if (a)
	{
		foo();
	}

	if (b)
		if (c)
			bar();
		else
			;

	else
	{
		foo();
	}
	switch (a)
	{
	case 1: break;
	case 2: break;
	default: break;
	}
	while (b-->0)
	{
		bar();
	}
	do
	{
		bar();
	} while (b-->0  );
}

enum FPP {
	FPP_ONE = 1,
	FPP_TWO = 2,
};

struct narg {
	int abc;
	char def;
	const char *ghi;
};

void f2(void)
{
	{ i++; }

	for (;;);

	for (;;) {   }
}

int main(int argc, char *argv[])
{
	if( argc == 1 )
	{
		printf("one");
	}
	else if( argc == 2 )
	{
		printf("two");
	}
	else
	{
		printf("%d", argc);
	}
	return 0;
}
