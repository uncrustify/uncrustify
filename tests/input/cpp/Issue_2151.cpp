void f( int a )
{
        namespace C { enum { Value }; }
        const bool ok = ( a & C::Value ) && true;
}
