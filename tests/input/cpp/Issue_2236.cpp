class A
{
public:
    virtual void f11111111( int a, int b, int c )   = 0;
    virtual void f2( int* ptr2 = nullptr ) = 0;
    virtual void f2333( int* ptr3 = 3 ) = delete;
    void f244444( int* ptr4 = 4 ) = default;
};
