
class Foo : public Bar
{

};

class Foo2 :
public Bar
{

};

class GLOX_API ClientBase : public Class, public OtherClass,
        public ThridClass, public ForthClass
	{
	};

Foo::Foo(int bar) : someVar(bar), othervar(0)
{
}

Foo::Foo(int bar) : someVar(bar),
   othervar(0)
{
}

Foo::Foo(int bar)
: someVar(bar), othervar(0)
{
}

Foo::Foo(int bar) :
someVar(bar), othervar(0)
{
}

Foo::Foo(int bar) :
someVar(bar),
   othervar(0)
{
}

Foo::Foo(int bar)
: someVar(bar),
   othervar(0)
{
}
