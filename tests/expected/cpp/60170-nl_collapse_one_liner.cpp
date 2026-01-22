// Test nl_collapse_*_one_liner options

void test_if()
{
	if (x) { return 1; }

	if (a) { x = 1; }
	else { x = 2; }
}

void test_for()
{
	for (int i = 0; i < n; i++) { sum += i; }

	for (int i = 0; i < n; i++) { if (a[i]) { count++; } }
}

void test_while()
{
	while (running) { process(); }
}

void test_do()
{
	do { process(); } while (running);
}

// Should NOT collapse - multiple statements
void test_no_collapse()
{
	if (x)
	{
		a = 1;
		b = 2;
	}

	for (int i = 0; i < n; i++)
	{
		sum += i;
		count++;
	}
}

// Should NOT collapse - contains comment
void test_no_collapse_comment()
{
	if (x)
	{
		// comment
		return 1;
	}
}
