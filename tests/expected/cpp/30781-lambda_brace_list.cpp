template<typename T, typename U>
auto add(T t, U u) -> decltype(t + u) { return t + u; }

int main()
{
	auto f1 = [&]() { return 1; };
	auto f2 = [&]() -> decltype(auto) { return 2; };
	string s1{'a', 'b'};
}
