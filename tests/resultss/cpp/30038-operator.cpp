
struct bar;
struct foo
{
   operator bar*();
};

class Foo {
   Foo        operator+(const Foo& rhs) const;

   const Foo& operator==(Foo& me);

   bool       operator>(const Foo& rhs) const;

   InStream&  operator<<(InStream& in);
}

const Foo& Foo::operator==(Foo& me)
{
}

Foo Foo::operator+(const Foo& rhs) const
{
}

bool Foo::operator>(const Foo& rhs) const
{
}

class Example
{
   char          m_array[256];

   Example&      operator=(const Example& rhs);
   Example&      operator+=(const Example& rhs);
   const Example operator+(const Example& other) const;
   bool          operator==(const Example& other) const;
   bool          operator!=(const Example& other) const;
   Example       operator+(const Example& x, const Example& y);
   Example       operator*(const Example& x, const Example& y);

   double&       operator()(int row, int col);
   double        operator()(int row, int col) const;
   void          operator++();
   int&          operator*();
   Example&      operator++();    // prefix ++
   Example       operator++(int); // postfix ++

   bool          operator<(const Example& lhs, const Example& rhs) const;

   int operator()(int index)
   {
      i = ~~3;
      return index + 1;
   }

   char& operator[](unsigned i)
   {
      return m_array[i & 0xff];
   }
}
bool Example::operator==(const Example& other) const
{
   /*TODO: compare something? */
   return false;
}
bool Example::operator!=(const Example& other) const
{
   return !operator==(other);
}


void a() {
   Op op = &X::operator==;
   if (!A)
      if (op != &X::operator==)
	 A(1) = a;
   if (!A) {
      if (op != &X::operator==)
	 A(1) = a;
   }
}

void *operator new(std::size_t) throw(std::bad_alloc);
void *operator new[](std::size_t) throw(std::bad_alloc);
void  operator delete(void *) throw();
void  operator delete[](void *) throw();
