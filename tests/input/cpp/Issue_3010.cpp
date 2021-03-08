namespace SomeLongNamespaceName {
class Foo { };
}

class Bar : SomeLongNamespaceName::Foo {
public:
Bar()
        : SomeLongNamespaceName::Foo(),
             myNumber(3), // <-- this line
             myOtherNumber(5)
{
}
private:
int myNumber;
int myOtherNumber;
};
