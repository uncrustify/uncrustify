void test()
{
	[]{}();
	[]{ foo(); }();
	[x]{ foo(x); }();
	[](int x){ foo(x); }(42);
	[y](int x){ foo(x, y); }(42);
	bar([]{ return 1; }());
	bar([]{ return foo(); }());
	bar([x]{ return foo(x); }(42));
	bar([](int x){ return foo(x); }(42));
	bar([y](int x){ return foo(x, y); }(42));

	[]  {}  ();
	[]  { foo(); }  ();
	[x]  { foo(x); }  ();
	[]  (int x){ foo(x); }  (42);
	[y]  (int x){ foo(x, y); }  (42);
	bar([]  { return 1; }  ());
	bar([]  { return foo(); }  ());
	bar([x]  { return foo(x); }  (42));
	bar([]  (int x){ return foo(x); }  (42));
	bar([y]  (int x){ return foo(x, y); }  (42));
}
