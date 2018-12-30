namespace A {
namespace S {

class C
{
public:
    virtual ~C()
    {
    }

    virtual void addSearch(const int &col) = 0;

    virtual void removeSearch(int id) = 0;
};

} // namespace S
} // namespace A

namespace B {

// This is a comment!
class D
{
public:
    D();
};

} // namespace B

// This is also a comment!
class E
{
public:
    E();
};
