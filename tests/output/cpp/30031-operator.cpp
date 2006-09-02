class Foo {
Foo operator+(const Foo& rhs) const;

const Foo& operator ==(Foo& me);

bool operator>(const Foo& rhs) const;

InStream& operator <<(InStream& in);
}

const Foo& Foo::operator ==(Foo& me)
{
}

Foo Foo::operator+(const Foo& rhs) const
{
}

bool Foo::operator>(const Foo& rhs) const
{
}
