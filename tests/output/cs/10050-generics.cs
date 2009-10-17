
int foo()
{
	a.b<c, d>();
	a.b<c, e<d>>();
	a.b<c, d>();
	a.b<c, e<d>>();
	a.b<c, e<d>>();

	return default(T);
}
