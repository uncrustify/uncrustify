union U
{
    void Foo();

    U();

    ~U();

};

struct S
{
    void Foo();

    S();

    ~S();

};

struct Outer
{
    union Inner
    {
        void Foo();

    };

    union
    {
        void Anon();

        int x;
    };

    void Keep();

};

union Top
{
    struct In
    {
        void Foo();

    };

    class CIn
    {
        void Foo();

        CIn();

    };

    void Bar();

    Top();

};

namespace
{
    union AU
    {
        void Foo();

        AU();

    };
}

namespace N::M
{
    union NU
    {
        void Foo();

        NU();

    };
}

template <class T> union TU
{
    void Foo();

    TU();

};

void fn()
{
    union LU
    {
        void Foo();

        LU();

    };
}

union Shapes
{
    int *ptr();

    const char *name() const;

    int &ref();

    bool operator==(const Shapes&) const;

    static void sf();

    explicit Shapes(int);

    template <class T> void set(T);

    void lone(Foo);

    void two(Foo, Bar);

    constexpr Shapes() noexcept;

    [[deprecated]] void legacy();

    Shapes(const Shapes&) = delete;

    friend void Fr();

    friend class X;
    ~Shapes() noexcept;

    void last();

};
