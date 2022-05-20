#define emit /* syntactic sugar, like in Qt */

void some_signal(int *x)
{
}

int main(void)
{
	int x;
	emit some_signal(&x);
	return 0;
}
