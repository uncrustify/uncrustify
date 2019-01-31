class Foo
{
public:
Foo( int bar = 1 );
Foo( const Foo & )             = delete;
Foo &operator= ( const Foo & ) = delete;
~Foo();
};
