int foo()
{
	try { foo(bar); }catch (int *e) { return 0; }

	if (false) try { throw int(); }catch(...) {}

	return 1;
}
