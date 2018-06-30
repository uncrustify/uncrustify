using x = Foo::foo_t;

using a1 = decltype( &Foo::operator() );
using a2 = Bar<decltype( &Foo::operator() )>;

using b1 = decltype( *Foo::y );
using b2 = Bar<decltype( *Foo::y )>;
