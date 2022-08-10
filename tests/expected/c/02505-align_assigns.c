unsigned foo(unsigned in1, unsigned in2, unsigned in3, unsigned in4) {
	unsigned a     = 0xa;
	unsigned b     = 0xb;
	unsigned c     = 0xc;
	unsigned x[10] = {0};
	{
		unsigned xyz = (
			1
			+
			2
			+
			3
			+
			4
			+
			5
			);
	}
	unsigned r1       = in1 * in3;
	unsigned r2first  = in1 * in4;
	unsigned r2second = in2 * in3;
	unsigned r3       = in2 * in4;

	a            ^=
		b; c += d;



	a             += (
		b); c += d;



	a             += x[
		0]; c += d;



	a    += b; {
	}; c += d;

	return r1 * r2first + r3 * r2second + a * b + c;
}
