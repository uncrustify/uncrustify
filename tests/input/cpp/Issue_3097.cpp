void foo()
{
    for( unsigned p = 0; p < np;
         ++p )
    {
      double* o = bar[p];
    }
    int x = 42;
}

void bar()
{
    // hello
    int x  = 42;
    if( x ) foo;
}
