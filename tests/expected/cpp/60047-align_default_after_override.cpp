class B
{
public:
B()          = default;
virtual ~B() = default;
};

class D1 : public B
{
public:
D1()                      = default;
~D1()                     = default;
D1(const D1&)             = delete;
D1(D1&&)                  = delete;
D1& operator=(const D1&)  = delete;
D1& operator=(const D1&&) = delete;
};

class D2 : public B
{
public:
D2()                     = default;
~D2() override           = default;
D2(const D2&)            = delete;
D2(D2&&)                 = delete;
D2& operator=(const D2&) = delete;
D2& operator=(D2&&)      = delete;
};
