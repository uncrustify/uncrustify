
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

ClientBase :: ClientBase (const std::string& ns,
   const std::string& ns1,
   const std::string& ns2)
{

}

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
