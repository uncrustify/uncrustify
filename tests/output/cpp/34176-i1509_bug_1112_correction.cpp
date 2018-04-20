void f()
{
	int i  = A::B::C::bar();
	int ii = A::B::C::bar();
}

int A::foo()
{
	return 1;
}
int A::B::foo()
{
	return A::foo();
}
int A::B::C::foo()
{
	return A::B::foo();
}
int A::B::C::D::foo()
{
	return A::B::C::foo();
}
int A::B::C::D::E::foo()
{
	return A::B::C::D::foo();
}
