
struct bar;
struct foo
{
   operator bar*();
};

class Foo {
   Foo operator +(const Foo& rhs) const;

   const Foo& operator ==(Foo& me);

   bool operator >(const Foo& rhs) const;

   InStream& operator <<(InStream& in);
}

const Foo& Foo::operator ==(Foo& me)
{
}

Foo Foo::operator +(const Foo& rhs) const
{
}

bool Foo::operator >(const Foo& rhs) const
{
}

class Example
{
   char m_array[256];

   operator Foo::Bar();
   operator Foo::Bar*();
   operator Foo::Bar& ();

   int operator () (int index)
   {
      i = ~~3;
      return index + 1;
   }

   int operator [](int index)
   {
      return m_array[index & 0xff];
   }
}
