#include <iostream>
struct s {int i;};

int main()
{
	struct s x;
	x.i = 5;
	std::cout << x.i << std::endl;
}       // main
