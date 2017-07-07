#include <iostream>

template<size_t T>
class MyFoo
{
public:
   MyFoo()
   {
      std::cout << T << std::endl;
   }
};

int main()
{
   const size_t mySize = INT8_MAX* 2;
   MyFoo<mySize * 2> foo1;
   MyFoo<mySize/2> foo2;
   MyFoo<2*mySize> foo1;
   MyFoo<2/mySize> foo2;
}
