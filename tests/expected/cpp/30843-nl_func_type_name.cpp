
// zero
// one
//  two
//   three
void foo(void);

struct A
{
public:
	long_complicated_type f();
	A&                    operator+(const A& other);
};

A&
A::operator+(const A& other)
{
}

B
operator+(const B& other)
{
}

B
foo(const B& other)
{
}

class A
{
public:
explicit A(int);
int
aFunct() {
	return a;
}
int bFunc();
};

// Another file
int
A::bFunc()
{
// some code
}

template<typename T>
typename Foo<T>::Type
Foo<T>::Func()
{
}

void
Foo::bar() {
}

namespace foo {
Foo::Foo() {
}
}

Foo::~Foo() {
}

class Object
{
~Object(void);
};

template <class T>
void
SampleClassTemplate<T>::connect()
{
}

template <>
inline void
bar<MyType>(MyType r)
{
	foo(r);
}

template <T>
inline void
baz<>(T r)
{
	foo(r);
}
