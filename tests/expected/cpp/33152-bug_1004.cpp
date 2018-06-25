int main()
{
	int  b = 3;
	int* p = &b;

	// Should stay as b * *p
	int a = b * *p;

	// Correctly formats as a * b;
	int c = b * a;

	// Correctly formats as d = *p;
	int d = *p;
}
