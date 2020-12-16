namespace A {

namespace S {

class C
{
public:
    virtual ~C()
    {
    }

    virtual void addSearch(const int &col) = 0;

    virtual void removeSearch(int id) = 0;
};

} // namespace S

} // namespace A

namespace B {

// This is a comment!
class D
{
public:
    D();
};

} // namespace B

// This is also a comment!
class E
{
public:
    E();
};

namespace F {
}

void foo();

class G
{
};

void bar();

void foo2();

namespace E
{
}

void bar2();

void foo3();

namespace F
{
}

void bar3();

void foo4();

class I
{
};

using namespace F;

namespace M
{
void bar4();

/* multiline test comment
   before class */
template<typename ... Args>
// test comment between template specification and associated class
class H
{
    // nested class
    template<typename ...>
    friend class I;
    friend class J;

    // nested class K
    template<typename T>
    class K
    {

	// double-nested class L
	class L { };

    };

};

}

class AA;
class AB;

namespace BA
{
class BB;
class BC;

class BD
{
public:
    friend class BE;
    BD();
};

}
