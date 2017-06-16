//example for case in a switch statement
switch( ... )
{
case 1:
case 2:
{
    int v;
    ...
}
break;

    #if ( USE_FIVE )
case 3:
    doFive();
    break;
    #endif

default:
    break;
}