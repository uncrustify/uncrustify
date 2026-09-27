// Issue #4782, part B: a '^' following a value-template instantiation's
// closing '>' must stay a bitwise XOR, not be misclassified as a pointer/
// handle type, when it is embedded inside an expression rather than
// genuinely starting a declaration.

template<int N> constexpr int Mask = N;
template<int N> int GetMask() { return N; }

int Case1()
{
	int y = Mask<4> ^ Mask<8>;
	return y;
}

int Case2()
{
	int y = Mask<4> ^ 0xFF;
	return y;
}

int Case3()
{
	int y = Mask<4> ^ SOME_MACRO;
	return y;
}

int Case4()
{
	int y = Mask<4> ^ GetValue();
	return y;
}

int Case5()
{
	int x = Mask<4> ^ [](){ return 1; }();
	return x;
}

int Case6()
{
	Mask<4> ^= 8;
	return 0;
}

int Case7()
{
	int y = GetMask<4>() ^ GetMask<8>();
	return y;
}

int Case8()
{
	return Mask<4> ^ Mask<8>;
}

static array<int>^ s_Data;
