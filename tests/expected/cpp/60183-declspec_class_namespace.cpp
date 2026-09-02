class Foo
{
public:
    __declspec(dllexport) static int Bar;
    __declspec(dllexport) static int BazQux;
};

namespace N
{
    __declspec(dllexport) int Bar = 1;
    __declspec(dllexport) int BazQux = 2;
}
