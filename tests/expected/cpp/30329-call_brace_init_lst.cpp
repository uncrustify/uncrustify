void bar()
{
	foo(42, {1, 2, 3, 4});
	foo(42,
	    {1, 2, 3, 4});

	foo(42, vector{1, 2, 3, 4});
	foo(42,
	    vector{1, 2, 3, 4});
	foo(42, vector{1, 2, 3, 4});

	foo(42, vector<int>{1, 2, 3, 4});
	foo(42,
	    vector<int>{1, 2, 3, 4});
	foo(42, vector<int>{1, 2, 3, 4});
	foo(42, vector
	    <int>{1, 2, 3, 4});

	foo(42, decltype(something) {1, 2, 3, 4});
	foo(42,
	    decltype(something) {1, 2, 3, 4});
	foo(42, decltype(something) {1, 2, 3, 4});
}
