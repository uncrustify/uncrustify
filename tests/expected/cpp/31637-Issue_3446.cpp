Foo::
Foo() {
}

std::true_type blarg();
template <typename T>
decltype(std::declval<T &>().put(foo, bar), std::true_type())
has_module_api_(T && t);

void
foo()
{
	using V = decltype(STD::declval<T &>().put(foo, bar), std::true_type());
}

template <typename T>
decltype(std::declval<T &>()./* ((( */ put(foo, bar), std::true_type())
has_module_api_(T && t);
