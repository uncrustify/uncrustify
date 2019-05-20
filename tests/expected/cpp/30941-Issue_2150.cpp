int f( int a )
{
        switch ( a )
        {
        case 1:
        {
                return a;
        }
        case 2:
#if 1
        case 3:
#endif
        {
                return a;
        }
        }
}
