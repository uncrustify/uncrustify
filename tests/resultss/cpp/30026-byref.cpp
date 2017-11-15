bool foo(int &idx)
{
   if (idx < m_count)
   {
      idx++;
      return m_bool[idx-1];
   }
   return false;
}

class Foo {
public:
   Foo();
   Foo(const Foo &f);
};

class NS::Foo {
public:
   Foo(Bar &b);
};

template<  class T >  class ListManager
{
protected:
   T head;

public:
   ListManager()
   {
      head.next = head.prev = &head;
   }

   ListManager(const ListManager &ref)
   {
      head.next = head.prev = &head;
   }
}

const Foo &Foo::operator ==(Foo &me){
   ::sockaddr* ptr = (::sockaddr*)&host;
   return me;
}

MyType &MyClass::myMethode() {
   const MyType &t = getSomewhere();
}
