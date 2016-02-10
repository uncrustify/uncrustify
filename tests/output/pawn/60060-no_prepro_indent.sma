#if IS_DEBUG_ENABLED > 0
    #define DEBUG_LOGGER(%1) debugMesssageLogger( %1 );
#endif

#if IS_DEBUG_ENABLED > 0
#define DEBUG_LOGGER(%1) debugMesssageLogger( %1 );
#endif

/**
 * Contains all unit tests to execute.
 */
#define ALL_TESTS_TO_EXECUTE \
{ \
    test_register_test(); \
    test_gal_in_empty_cycle1(); \
    test_gal_in_empty_cycle2(); \
    test_gal_in_empty_cycle3(); \
    test_gal_in_empty_cycle4(); \
    test_is_map_extension_allowed1( true ); \
}

/**
 * Write debug messages to server's console accordantly with cvar gal_debug.
 * If gal_debug 1 or more higher, the voting and runoff times are set to 5 seconds.
 *
 * @param mode the debug mode level, see the variable 'g_debug_level' for the levels.
 * @param text the debug message, if omitted its default value is ""
 * @param any the variable number of formatting parameters
 */
stock debugMesssageLogger( mode, message[], any: ... )
{
    if( mode & g_debug_level )
    {
        static formated_message[ LONG_STRING ]

        vformat( formated_message, charsmax( formated_message ), message, 3 )

        server_print( "%s",                      formated_message )
        client_print( 0,    print_console, "%s", formated_message )
    }
}

