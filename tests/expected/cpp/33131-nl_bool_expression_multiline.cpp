#include <iostream>
#include <vector>
#include <string>

bool example_function(int a, int b, int c, int d) {
	if (a > 10 &&
	    b < 5 &&
	    c == 3 &&
	    d != 0 &&
	    a * d > b + c) {
		return true;
	}
	return false;
}

bool another_function(int x, int y, int z) {
	return (x == y ||
	        y == z ||
	        x + y > z * 2 ||
	        x - z < y * 3 ||
	        x > 100);
}

class ExampleClass {
public:
bool member_function(const std::vector<int>& vec) const {
	return (vec.size() > 5 &&
	        vec[0] == 10 &&
	        vec.back() < 20);
}

bool complex_condition(int a, int b,
                       const std::string& str) const {
	return (a > b ||
	        str.empty() ||
	        str.find("test") != std::string::npos);
}
};

int main() {
	int a = 15, b = 3, c = 3, d = 2;
	int x = 5, y = 5, z = 10;
	std::vector<int> vec = {10, 2, 3, 4, 5, 6};
	std::string str = "this is a test string";

	ExampleClass example;

	if (example_function(a, b, c, d) &&
	    another_function(x, y, z)) {
		std::cout << "Both conditions are true." <<
		        std::endl;
	}

	if (a < b ||
	    b == c ||
	    d > 100 ||
	    a * b - d < c * c + b) {
		std::cout << "Something is true." <<
		        std::endl;
	}

	if (example.member_function(vec) &&
	    example.complex_condition(a, b, str)) {
		std::cout <<
		        "Class member function conditions are true."
		          << std::endl;
	}

	return 0;
}
