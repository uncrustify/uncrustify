void test(int var)
{
	switch (var)
	{
#define CASE(VAL) \
		case VAL:     \
			{
	CASE(1) break;
	}
	CASE(2) break;
}

CASE(3) break;
}

	default:
		break;
}
}
