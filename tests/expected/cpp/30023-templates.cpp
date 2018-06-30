#include <list>
#include <map>
#include <vector>

#define MACRO(T)    f<T>()

class MyClass
{
 public:
   std::map<int, bool>             someData;
   std::map<int, std::list<bool> > otherData;
};

void foo()
{
   List<byte> bob = new List<byte>();
}

A<B>     foo;
A<B, C>  bar;
A<B *>   baz;
A<B<C> > bay;

void asd(void)
{
   A<B>     foo;
   A<B, C>  bar;
   A<B *>   baz;
   A<B<C> > bay;
   if (a < b && b > c)
   {
      a = b < c > 0;
   }
   if (a<bar()> c)
   {
   }
   a < up_lim() ? do_hi() : do_low;
   a[a<b> c] = d;
}

template<typename T> class MyClass
{
}

template<typename T>
class MyClass
{
}

template<typename A, typename B, typename C> class MyClass : myvar(0),
   myvar2(0)
{
}

template<typename A, typename B, typename C> class MyClass
   : myvar(0),
   myvar2(0)
{
}


static int max_value()
{
   return((std::numeric_limits<int>::max)());
}

template<class Config_>
priority_queue<Config_>::~priority_queue ()
{
}

template<class T>
T test(T a)
{
   return(a);
}

int main()
{
   int k, j;
   h   g<int>;

   k = test<int>(j);
   return(0);
}

template<typename T, template<typename, unsigned int, unsigned int> class ConcreteStorageClass>
class RotationMatrix
   : public StaticBaseMatrix<T, 3, 3, ConcreteStorageClass>
{
 public:
   RotationMatrix()
      : StaticBaseMatrix<T, 3, 3, ConcreteStorageClass>()
   {
      // do some initialization
   }

   void assign(const OtherClass<T, 3, 3>& other)
   {
      // do something
   }
};

int main()
{
   MyClass<double, 3, 3, MyStorage> foo;
}

template<typename CharT, int N, typename Traits>
inline std::basic_ostream<CharT, Traits>& FWStreamOut(std::basic_ostream<CharT, Traits>&os,
                                                      const W::S<CharT, N, Traits>&s)
{
   return(operator<<<CharT, N, Traits, char, std::char_traits<char> >(os, s));
}

struct foo
{
   type1<int&> bar;
};
struct foo
{
   type1<int const> bar;
};


template<int i> void f();
template<int i> void g()
{
   f<i - 1>();
   f<i>();
   f<i + 1>();
   f<bar()>();
}
void h()
{
   g<42>();
}

#include <vector>
std::vector<int> A(2);
std::vector<int> B;
std::vector<int> C(2);
std::vector<int> D;

template<class T> struct X
{
   template<class U> void operator()(U);
};

template<class T> class Y {
   template<class V> void f(V);
};

void                        (*foobar)(void) = NULL;
std::vector<void (*)(void)> functions;

#define MACRO(a)    a
template<typename = int> class X;
MACRO(void f(X<>& x));
void g(X<>& x);

#include <vector>
typedef std::vector<std::vector<int> >    Table; // OK
typedef std::vector<std::vector<bool> >   Flags; // Error

void func(List<B>        =default_val1);
void func(List<List<B> > =default_val2);

BLAH<(3.14 >= 42)> blah;
bool               X = j<3> > 1;

void foo()
{
   A<(X > Y)> a;
   a = static_cast<List<B> >(ld);
}

template<int i> class X {   /* ... */
};
X < 1 > 2 > x1;             // Syntax error.
X<(1 > 2)> x2;              // Okay.

template<class T> class Y { /* ... */
};
Y<X<1> >        x3;         // Okay, same as "Y<X<1> > x3;".
Y<X<(6 >> 1)> > x4;


template<typename T>
int
myFunc1(typename T::Subtype val);

int
myFunc2(T::Subtype val);
