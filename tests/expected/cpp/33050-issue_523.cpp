
#define MACRO(templ_type) template <typename T> class Abc<templ_type<T> > { }

template<typename T> class Foo<Bar<T> > { };
