#include <amxmodx>

#define COOL_MACRO(%1,%2) ( % 1 + % 2 + 1 )

#define SXO( % 1, %2 ) 1 < %1 \
    || %2 > 2

public plugin_init()
{
    for( new i = 0; i < 5; i++ )
    {
        crazy_var = crazy_var + 5 % ( 18 * 15 ) / 26
        crazy_var = crazy_var + 5 % ( 18 * 15 ) /  26
        server_print( "(%i) Test2: %i Test1: %i", i, Test2(), Test1() );
    }

    if( SXO( Test2(), Test1() ) )
    {
        COOL_MACRO( Test2(), Test1() )
    }
}

Test1()
{
    static var;
    var++
    #emit CONST.pri 1337
    #emit CONST. pri 1337
    return var;
}

Test2()
{
    #emit CONST .pri 1911
    #emit CONST.pri 1911
    static var;
    return --var
}

