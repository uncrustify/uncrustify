#include <string>

struct Foo
{
   std::string name;
   int value;
};

struct Bar
{
   Foo* parent;
   int modifier;
};

void baz( Foo*, Bar* );
