#include <type_traits>

template< typename ... > struct wrap;
int value();

template< typename F, typename T >
struct call_types {
   using plain = decltype(std::declval < F > ().template operator () < T >());
   using nested = wrap < decltype(std::declval < F > ().template operator () < T >()) >;
   using twice = wrap < wrap < decltype(std::declval < F > ().template operator () < T >()) > >;
   using comment = wrap < decltype(std::declval < F > ().template operator /* call */ () /* args */ < T >()) >;
   using comparison = wrap < decltype(value()<1) >;
   using compared_call = wrap < decltype(std::declval < F > ().template operator () < T >()<1) >;
   using no_template = wrap < decltype(std::declval < F > ().operator ()()<1) >;
};
