union U
{
    void Foo ();
    U ();
    ~U ();
    void Def () {
    }
    U (int) {
    }
    ~U () {
    }
    friend void Fr ();
    friend U operator+ (const U&, const U&);
    auto get () -> int;
    int *ptr ();
};

struct S
{
    void Foo ();
    S ();
    ~S ();
    void Def () {
    }
    S (int) {
    }
    ~S () {
    }
    friend void Fr ();
    friend S operator+ (const S&, const S&);
    auto get () -> int;
    int *ptr ();
};
