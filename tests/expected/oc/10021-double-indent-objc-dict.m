id a = @{
	@"a": @1,
	@"b": @2,
};

struct foo_t b = {
	1,
	2,
};

SomeObject *build()
{
	return @{
		@"a": @1,
		@"b": @2,
	};
}
