int foo1()
{
	// should not have newline before '.'
	return std::pair<int, int>{1, 2}.first;
}

int foo2()
{
	// should be ARITH, not ADDR
	return int{3} & 2;
}

int foo3()
{
	// should be ARITH, not ADDR
	constexpr static int x = 3;
	return decltype(x){x} & 2;
}
