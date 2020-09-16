void foo()
{
	// Ensure non-shift operators aren't changed
	x = 1 +
	    2;
	x = 1
	    + 2;
	x = 1 + 2;

	// Test position of shift operator
	cout << x << y;
	cout << x << y;
	cout << x << y;
}
