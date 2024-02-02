template <typename T>
int foo()
{
    return T{}.template method<arg>();
}
