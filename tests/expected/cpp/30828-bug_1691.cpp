#include <string>

std::string foo()
{
	return std::string{"abc"};
}
int main()
{
	const std::string&& name1 = foo();
	std::string&&       name2 = foo();

	const auto&& name3 = foo();
	auto&&       name4 = foo();
}
