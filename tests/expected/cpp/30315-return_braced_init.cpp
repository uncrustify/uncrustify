int foo()
{
	// should not have newline before '.'
	return std::pair<int, int>{1, 2}.first;
}
