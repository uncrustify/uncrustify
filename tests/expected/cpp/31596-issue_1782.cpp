using a1 = decltype( bar() );
using b1 = decltype( bar< int >() );
using c1 = decltype( foo::bar< int >() );
using d1 = decltype( *( bar< int >() ) );
using e1 = decltype( *( foo::bar< int >() ) );

using a2 = decltype( bar() );
using b2 = decltype( bar< int >() );
using c2 = decltype( foo::bar< int >() );
using d2 = decltype( *( bar< int >() ) );
using e2 = decltype( *( foo::bar< int >() ) );

using a3 = decltype( bar(0) );
using b3 = decltype( bar< int >(0) );
using c3 = decltype( foo::bar< int >(0) );
using d3 = decltype( *( bar< int >(0) ) );
using e3 = decltype( *( foo::bar< int >(0) ) );

using x1 = decltype( ( 0 ) );
using x2 = decltype( ( 0 ) );
