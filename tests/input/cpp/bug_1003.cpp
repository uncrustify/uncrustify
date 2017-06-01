class Foo
{
public:
	Foo(
		int x_,
		int y_
	) : x(x_), y(y_)
	{
	}
private:
	int x;
	int y;
};

class Bar
{
public:
	// Splits 3,5 onto newlines
	Bar() : Bar(3, 5)
	{
	}

	// No split here
	Bar(
		int x,
		int y
	) : foo(x, y)
	{
	}

	Foo foo;
};

