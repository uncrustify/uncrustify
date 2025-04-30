enum MyEnum
{
	Null = 0,
	FlagA = 1,
	FlagB = 2,
};

int main()
{
	MyEnum flags = MyEnum::Null;

	MyEnum* flagsPtr = &flags;
	flagsPtr = &flags;

	if ((flags & MyEnum::FlagA) != MyEnum::Null)
	{
		return 1;
	}

	if ((flags & MyEnum::FlagB) != MyEnum::Null)
	{
		return 1;
	}

	if ((flags & MyEnum::FlagB) != MyEnum::Null)
	{
		return 1;
	}

	if (((flags & MyEnum::FlagA)) != MyEnum::Null)
	{
		return 1;
	}

	if (((flags & MyEnum::FlagA)) != MyEnum::Null)
	{
		return 1;
	}

	if (((flags & MyEnum::FlagA)) != MyEnum::Null)
	{
		return 1;
	}

	return 0;
}
