class Foo
{
public:
Foo( int bar)                  = 0;
Foo( int bar                   = 777 );
Foo( const Foo & )             = delete;
Foo( int boo )                 = default;
Foo( unsigned int )            = default;
Foo( unsigned int boo          =999 );
Foo &operator= ( const Foo & ) = delete;
~Foo();
};
