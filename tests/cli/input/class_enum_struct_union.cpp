   /**
    * the enum (and variable declarations thereof) could be of
    * the following forms:
    *
    * "enum type [: integral_type] { ... } [x, ...]"
    * "enum type [: integral_type]"
    * "enum class type [: integral_type] { ... } [x, ...]"
    * "enum class type [: integral_type]"
    * "enum [: integral_type] { ... } x, ..."
    */

   /**
    * the class/struct (and variable declarations thereof) could be of
    * the following forms:
    *
    * template<...> class/struct[<...>] [macros/attributes ...] type [: bases ...] { }
    * template<...> class/struct[<...>] [macros/attributes ...] type
    * class/struct[ [macros/attributes ...] type [: bases ...] { } [x, ...]
    * class/struct [macros/attributes ...] type [x, ...]
    * class/struct [macros/attributes ...] [: bases] { } x, ...
    */

#define ALIGNAS(byte_alignment) __attribute__((aligned(byte_alignment)))

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#define API_EXPORT __attribute__ ((visibility("default")))
#elif defined _WIN32
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT
#endif

namespace outer_namespace
{

namespace inner_namespace
{

class Base1 { };

template<typename> class Base2 { };

}

}

// template<...> class/struct[<...>] [macros/attributes ...] type : bases ... { }
template<typename, typename ...>
class API_EXPORT __attribute__((__deprecated__)) ALIGNAS(4) c1
: public outer_namespace::inner_namespace::Base1,
  public outer_namespace::inner_namespace::Base2<outer_namespace::inner_namespace::Base1>
{

};

// template<...> class/struct[<...>] [macros/attributes ...] type { }
template<typename, typename ...>
class API_EXPORT c2
{
public:

   template<typename T>
   struct inner_class
   {
      static inner_class<T> *m_inner_class;
   };
};

template<> template<> struct API_EXPORT c2<int>::inner_class<int> *c2<int>::inner_class<int>::m_inner_class = nullptr;

// template<...> class/struct[<...>] [macros/attributes ...] type
template<typename, typename ...>
class API_EXPORT c2;

// class/struct [macros/attributes ...] type : bases ... { } x, ...
class API_EXPORT __attribute__((__deprecated__)) ALIGNAS(4) c3
: public outer_namespace::inner_namespace::Base2<int>,
  public c2<int>::inner_class<int>
{
public:
   c3(int x = 0, int y = 0, int z = 0) : m_x(x), m_y(y), m_z(z) { }

   int m_x;
   int m_y;
   int m_z;
} c31, *c32 = nullptr, *c33[] = { nullptr, nullptr }, c34{ 0, 1, 2}, * const c35(nullptr), c16(0, 1, 2);

// class/struct [macros/attributes ...] type x, ...
class __attribute__((__deprecated__)) API_EXPORT ALIGNAS(4) c3 c41, *c42 = c32 ? c32 : nullptr, *c43[] = { nullptr, nullptr }, c44{ 0, 1, 2}, * const c45(nullptr), c46(0, 1, 2);

// class/struct [macros/attributes ...] type : bases ... { } x, ...
class ALIGNAS(4) API_EXPORT __attribute__((__deprecated__))
: public outer_namespace::inner_namespace::Base1
{
public:
   int m_x;
   int m_y;
   int m_z;
} c51, *c52 = nullptr, *c53[] = { nullptr, nullptr };


// enum type : integral_type { ... } x, ...
enum e1 : long long { a1, b1, d1 } e11, e12, e13;

// enum type : integral_type { ... }
enum e2 : unsigned int { a2, b2, d2 };

// enum type : integral_type
enum e3 : short;

// enum type x, ...
enum e3 e31, e32;

// enum class type : integral_type { ... } x, ...
enum class e4 : long long { a4, b4, d4 } e41, e42, e43, e44;

// enum class type : integral_type { ... }
enum class e5 : unsigned int { a5, b5, d5 };

// enum class type : integral_type
enum class e6 : short;

// enum class type
enum class e7;

// enum : integral_type { ... } x, ...
enum : long long { a8, b8, c8 } e81, e82;

// enum { ... } x, ...
enum { a9, b9, c9 } e91, e92;

union API_EXPORT u1 { int x; long y; } u11, *u12 = nullptr, *u13{0};

union API_EXPORT u1 u21;
