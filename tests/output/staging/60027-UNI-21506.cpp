struct bar
{
    void (Namespace::*method)(Class& param);
};

void Class::Foo(void (*callback)(const Class& entry))
{
}

void foo()
{
    bool a = 1;  // if you comment this out, the bug stops reproducing
}
