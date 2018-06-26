void f();
void g(int);
void h()
{
	f(/*foo*/);
	g(42 /*foo*/);
	g(/*foo*/ 42);
}
