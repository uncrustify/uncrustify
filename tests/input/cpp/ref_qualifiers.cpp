void foo()&;
void foo()&&;

void bar() const&;
void bar() const&&;

void baz() noexcept&;
void baz() noexcept&&;

void complex() const noexcept&;
void complex() const noexcept&&;

void func()&{
}

void func()&&{
}

void getValue() const&-> int;
void getValue()&&-> int;
