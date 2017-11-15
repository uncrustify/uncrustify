#define x struct z
#define max(a, b) ((a) > (b) ? (a) : (b))

void f()
{
	goto p;
p:
	goto q;
q:
	goto p;
}
