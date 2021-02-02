struct a {
  a() : a_(0) {}

  a() : a_(0)
  {}

  a() : a_(0)
  {
  }

  a() :
    a_(0) {
    }

  a() :
    a_(0) {}

  a() :
    a_(0)
  {}
};

a::a() : a_(0) {}

a::a() : a_(0)
{}

a::a() : a_(0)
{
}

a::a() :
  a_(0) {
  }

a::a() :
  a_(0) {}

a::a() :
  a_(0)
{
}

a::a() : a_(0)/**/ {}

a::a() : a_(0) /**/
{}

a::a() : a_(0) /**/
{
}

a::a() : a_(0) //
{}

a::a() : a_(0) //
{
}

a::a() :
  a_(0)/**/ {
  }

a::a() :
  a_(0)/**/ {}

a::a() :
  a_(0)//
{
}

a::a() :
  a_(0)/**/
{
}

a::a() {
}

a::a()
{
}

a::a() {}

a::a() /**/ {
}

a::a() /**/
{
}

a::a() //
{
}

a::a() /**/ {}

a::a() const : a_(0) {}

a::a() const : a_(0)
{}

a::a() const : a_(0)
{
}

a::a() const :
  a_(0) {
  }

a::a() const :
  a_(0) {}

a::a() const :
  a_(0)
{
}

void f(){}
void f()
{}

void f()const{}
void f()const
{}

void f()noexcept(){}
void f()noexcept()
{}

void f()/**/{}
void f()/**/
{}
void f()//
{}
