#include <vector>

class A
{
public:
    int a;
    int b;

    std::vector<int*> v =
    {
        &a,
        &b
    };
};
