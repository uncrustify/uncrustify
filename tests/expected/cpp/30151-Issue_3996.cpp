#include <iostream>

class OtherClass
{
public:
int i;
OtherClass() : i(5) {
}
OtherClass* self() {
	return this;
}
};

class MyClass
{
public:
OtherClass x;
int getI();
};

int MyClass::getI() {
	return this->x.self()->i;
}

int main() {
	MyClass c;
	std::cout << c.getI() << std::endl;
}
