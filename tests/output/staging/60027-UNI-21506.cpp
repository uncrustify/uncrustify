struct bar
{
    void (Namespace::*method)(Class& param);
};

void Class::Foo(void (*callback)(const Class& entry))
{
}

void foo()
{
    int a = 1;  // if you comment this out, the bug stops reproducing
}
