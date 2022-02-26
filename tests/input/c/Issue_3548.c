bool is_inside(int num)
{
	bool inside;

	inside = num >= 3
	   && num <= 10;

	return num <= 3
	   && num >= 10;
}

bool is_outside(int num)
{
	bool outside;

	outside = num < 3
	   || num > 10;

	return num < 3
	   || num > 10;
}
