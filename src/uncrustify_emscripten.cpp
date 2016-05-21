/*
 * uncrustify_emscripten.cpp
 *
 *  Created on: May 8, 2016
 *      Author: Daniel Chumak
 */

#ifdef EMSCRIPTEN

#include "prototypes.h"
#include "options.h"
#include "uncrustify_version.h"
#include "logger.h"
#include "log_levels.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include <emscripten/bind.h>


#define STRINGIFY( s ) # s


using namespace std;
using namespace emscripten;


extern void usage_exit( const char *msg, const char *argv0, int code );
extern int load_header_files();
extern const char *language_name_from_flags( int lang );
extern void uncrustify_file( const file_mem& fm, FILE *pfout,
                             const char *parsed_file,
                             bool defer_uncrustify_end = false);



// TODO: interface for args:
// unsure about these:
// -----------------------------------------------------------------------------
// --check
// --decode
// --parsed
// --detect
//
//
// will not be included:
// -----------------------------------------------------------------------------
// -t ( define via multiple --type )
// -d ( define via multiple --define)
// --assume ( no files available to guess the lang. based on the filename ending )
// --files ( no batch processing will be available )
// --prefix
// --suffix
// --assume
// --no-backup
// --replace
// --mtime
// --universalindent
// --help, -h, --usage, -?
//
//
// done:
// -----------------------------------------------------------------------------
// --update-config ( use show_config() )
// --update-config-with-doc ( show_config( bool withDoc = true ) )
// --version, -v ( use get_version() )
// --log, -L ( use log_set_sev( log_sev_t sev, bool value ) )
// -q ( use set_quiet() )
// --config, -c ( use set_config( string _cfg ) )
// --file, -f ( use uncrustify( string _file ) )
// --show-config( use show_options() )
// --show ( use show_log_type( bool ) )
// --frag ( use uncrustify( string _file, bool frag = true ) )
// --type ( use add_type( string _type ) )
// -l ( use set_language( int langIDX ) )


// TODO (upstream): it would be nicer to set settings via uncrustify_options enum_id
// but this wont work since type info is needed
// which is inside of the
// _static_ option_name_map<
// option_name : string,
// option_map_val : struct { type : argtype_e, ....} >
// to access the right union var inside of op_val_t
// even if option_name_map would not be static, no direct access to the type
// info is possible since the maps needs to be iterated to find the according
// enum_id
//
// int set_option_value(op_val_t option_id, const char *value)
// string get_option_value(op_val_t option_id )


enum class lang_flags : int
{
    LANG_C_ = LANG_C,
    LANG_CPP_ = LANG_CPP,
    LANG_D_ = LANG_D,
    LANG_CS_ = LANG_CS,
    LANG_VALA_ = LANG_VALA,
    LANG_JAVA_ = LANG_JAVA,
    LANG_PAWN_ = LANG_PAWN,
    LANG_OC_ = LANG_OC,
    LANG_OCPP_ = LANG_OC | LANG_CPP,
    LANG_ECMA_ = LANG_ECMA,
//TODO ???? see language_names[] uncrustify.cpp, what about flags ?
//    LANG_ALLC_ = LANG_ALLC,
//    LANG_ALL_ = LANG_ALL,
};


// TODO (upstream): use a named enum for languages
/**
 *  sets the language of the to be formatted text
 *
 *  @param langIDX: ID of the language, see enum lang_flags or
 *                  uncrustify_types.h
 */
void set_language( lang_flags langIDX )
{
    cpd.lang_flags = static_cast<int>(langIDX);
}


// TODO ( upstream ): this needs more than just adding types,
// add ways to add and remove keywords
/**
 * adds a new keyword to Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
 *
 * @param type: keyword that is going to be added
 */
void add_type( string type )
{
    if( type.empty() )
    {
        LOG_FMT( LERR, "%s: input string is empty\n", __func__ );
        return;
    }
    add_keyword( type.c_str(), CT_TYPE );
}


//! clears Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
void clear_keywords()
{
    clear_keyword_file();
}


/**
* Show or hide the severity prefix "<1>"
*
* @param b: true=show, false=hide
*/
void show_log_type( bool b )
{
    log_show_sev( b );
}


//! returns the UNCRUSTIFY_VERSION string
string get_version()
{
    return UNCRUSTIFY_VERSION;
}


//! disables all logging messages
void set_quiet()
{
    // set empty mask
    log_mask_t mask;

    log_set_mask( mask );
}


/**
* sets value of an option
*
* @param name:  name of the option
* @param value: value that is going to be set
* @return options enum value of the found option or -1 if option was not found
*/
int set_option( string name, string value )
{
    if( name.empty() )
    {
        LOG_FMT( LERR, "%s: name string is empty\n", __func__ );
        return -1;
    }
    if( value.empty() )
    {
        LOG_FMT( LERR, "%s: value string is empty\n", __func__ );
        return -1;
    }

    return set_option_value( name.c_str(), value.c_str() );
}


/**
* returns value of an option
*
* @param name: name of the option
* @return currently set value of the option
*/
string get_option( string name )
{
    if( name.empty() )
    {
        LOG_FMT( LERR, "%s: input string is empty\n", __func__ );
        return "";
    }

    const auto option = unc_find_option( name.c_str() );
    if( option == NULL )
    {
        LOG_FMT( LWARN, "Option %s not found\n", name.c_str() );
        return "";
    }

    return op_val_to_string( option->type, cpd.settings[option->id] );
}


//! returns a string with option documentation
string show_options()
{
    FILE*  stream;
    char*  buf;
    size_t len;

    // TODO (upstream): see uncrustify()
    stream = open_memstream( &buf, &len );
    if( stream == NULL )
    {
        LOG_FMT( LERR, "Failed to open_memstream\n" );
        fflush( stream );
        fclose( stream );
        free( buf );
        return "";
    }


    print_options( stream );
    fflush( stream );
    fclose( stream );

    string out( buf );
    free( buf );

    return out;
}


/**
* returns the config file string based on the current configuration
*
* @param withDoc:          false=without documentation,
*                          true=with documentation text lines
* @param only_not_default: false=containing all options,
*                          true=containing only options with non default values
* @return returns the config file string based on the current configuration
*/
string show_config( bool withDoc, bool only_not_default )
{
    FILE*  stream;
    char*  buf;
    size_t len;

    // TODO (upstream): see uncrustify()
    stream = open_memstream( &buf, &len );
    if( stream == NULL )
    {
        LOG_FMT( LERR, "Failed to open_memstream\n" );
        fflush( stream );
        fclose( stream );
        free( buf );
        return "";
    }

    save_option_file_kernel( stream, withDoc, only_not_default );

    fflush( stream );
    fclose( stream );

    string out( buf );
    free( buf );

    return out;
}


/**
* returns the config file string with all options based on the current configuration
*
* @param withDoc: false= without documentation, true=with documentation text lines
* @return returns the config file string with all options based on the current configuration
*/
string show_config( bool withDoc )
{
    return show_config( withDoc, false );
}


//!returns the config file string with all options and without documentation based on the current configuration
string show_config()
{
    return show_config( false, false );
}


/**
 * initializes the current libUncrustify instance,
 * used only for emscripten binding here and will be automatically called while
 * module initialization
*/
void initialize()
{
    register_options();
    set_option_defaults();
    log_init( stderr );

    LOG_FMT( LSYS, "Initialized libUncrustify\n" );
}


//! destroys the current libUncrustify instance
void destruct()
{
    clear_keyword_file();
    clear_defines();
}


/**
* reads option file string, sets the defined options
*
* @return returns EXIT_SUCCESS on success
*/
int loadConfig( string _cfg )
{
    if( _cfg.empty() )
    {
        LOG_FMT( LERR, "%s: input string is empty\n", __func__ );
        return EXIT_FAILURE;
    }

    unique_ptr<char[]> cfg(new char[_cfg.length() + 1]);
    std::strcpy( cfg.get(), _cfg.c_str() );

    if( load_option_fileChar( cfg.get() ) != EXIT_SUCCESS )
    {
        LOG_FMT( LERR, "unable to load the config\n" );
        return EXIT_FAILURE;
    }

    /* This relies on cpd.filename being the config file name */
    load_header_files();


    LOG_FMT( LSYS, "finished loading config\n" );
    return EXIT_SUCCESS;
}


/**
* format string
*
* @param file: string that is going to be formated
* @param frag: true=fragmented code input, false=unfragmented code input
* @return formated string
*/
string uncrustify( string file, bool frag )
{
    if( file.empty() )
    {
        LOG_FMT( LERR, "%s: file string is empty\n", __func__ );
        return "";
    }

    // Problem: uncrustify originally is not a lib and uses global vars such as
    // cpd.error_count for the whole program execution
    // to know if errors occurred during the formating step we reset this var here
    cpd.error_count = 0;

    if( cpd.lang_flags == 0 ) // 0 == undefined
    {
        LOG_FMT( LWARN, "language of input file not defined, C++ will be assumed\n" );
        cpd.lang_flags = LANG_CPP;
    }

    file_mem fm;
    fm.raw.clear();
    fm.data.clear();
    fm.enc = ENC_ASCII;
    fm.raw = vector< UINT8 >( file.begin(), file.end() );

    if( !decode_unicode( fm.raw, fm.data, fm.enc, fm.bom ) )
    {
        LOG_FMT( LERR, "Failed to read code\n" );
        return "";
    }

    cpd.filename = "stdin";

    /* Done reading from stdin */
    LOG_FMT( LSYS, "Parsing: %d bytes (%d chars) from stdin as language %s\n",
             (int) fm.raw.size(), (int) fm.data.size(),
             language_name_from_flags( cpd.lang_flags ) );

    cpd.frag = frag;

    FILE*  stream;
    char*  buf;
    size_t len;

    // TODO (upstream): uncrustify uses FILE instead of streams for its outputs
    // to redirect FILE writes into a char* open_memstream is used
    // windows lacks open_memstream, only UNIX/BSD is supported
    // apparently emscripten has its own implementation, if that is not working
    // see: stackoverflow.com/questions/10305095#answer-10341073
    stream = open_memstream( &buf, &len );
    if( stream == NULL )
    {
        LOG_FMT( LERR, "Failed to open_memstream\n" );
        fflush( stream );
        fclose( stream );
        free( buf );
        return "";
    }

    uncrustify_file( fm, stream, NULL );

    fflush( stream );
    fclose( stream );

    string out( buf );
    free( buf );

    if( cpd.error_count != 0 )
    {
        LOG_FMT( LWARN, "%d errors occurred during formating\n", cpd.error_count );
    }
    return out;
}


/**
* format string, assume unfragmented code input
*
* @param file: string that is going to be formated
* @return formated string
*/
string uncrustify( string file )
{
    // overload for default frag parameter
    return uncrustify( file, false );
}


EMSCRIPTEN_BINDINGS( MainModule )
{
    emscripten::function( STRINGIFY( show_config ), select_overload< string( bool, bool ) >( &show_config ) );
    emscripten::function( STRINGIFY( show_config ), select_overload< string( bool ) >( &show_config ) );
    emscripten::function( STRINGIFY( show_config ), select_overload< string() >( &show_config ) );
    emscripten::function( STRINGIFY( set_language ), &set_language );
    emscripten::function( STRINGIFY( clear_keywords ), &clear_keywords );
    emscripten::function( STRINGIFY( add_type ), &add_type );
    emscripten::function( STRINGIFY( show_log_type ), &show_log_type );
    emscripten::function( STRINGIFY( show_options ), &show_options );
    emscripten::function( STRINGIFY( initialize ), &initialize );
    emscripten::function( STRINGIFY( destruct ), &destruct );

    emscripten::function( STRINGIFY( get_version ), &get_version );
    emscripten::function( STRINGIFY( uncrustify ), select_overload< string( string, bool ) >( &uncrustify ) );
    emscripten::function( STRINGIFY( uncrustify ), select_overload< string( string ) >( &uncrustify ) );

    emscripten::function( STRINGIFY( loadConfig ), &loadConfig );
    emscripten::function( STRINGIFY( set_option ), &set_option );
    emscripten::function( STRINGIFY( get_option ), &get_option );

    emscripten::function( STRINGIFY( set_quiet ), &set_quiet );
    emscripten::function( STRINGIFY( log_set_sev ), &log_set_sev );

    enum_< log_sev_t >( STRINGIFY( log_sev_t ) )
    .value( STRINGIFY( LSYS ), LSYS )
    .value( STRINGIFY( LERR ), LERR )
    .value( STRINGIFY( LWARN ), LWARN )
    .value( STRINGIFY( LNOTE ), LNOTE )
    .value( STRINGIFY( LINFO ), LINFO )
    .value( STRINGIFY( LDATA ), LDATA )
    .value( STRINGIFY( LFILELIST ), LFILELIST )
    .value( STRINGIFY( LLINEENDS ), LLINEENDS )
    .value( STRINGIFY( LCASTS ), LCASTS )
    .value( STRINGIFY( LALBR ), LALBR )
    .value( STRINGIFY( LALTD ), LALTD )
    .value( STRINGIFY( LALPP ), LALPP )
    .value( STRINGIFY( LALPROTO ), LALPROTO )
    .value( STRINGIFY( LALNLC ), LALNLC )
    .value( STRINGIFY( LALTC ), LALTC )
    .value( STRINGIFY( LALADD ), LALADD )
    .value( STRINGIFY( LALASS ), LALASS )
    .value( STRINGIFY( LFVD ), LFVD )
    .value( STRINGIFY( LFVD2 ), LFVD2 )
    .value( STRINGIFY( LINDENT ), LINDENT )
    .value( STRINGIFY( LINDENT2 ), LINDENT2 )
    .value( STRINGIFY( LINDPSE ), LINDPSE )
    .value( STRINGIFY( LINDPC ), LINDPC )
    .value( STRINGIFY( LNEWLINE ), LNEWLINE )
    .value( STRINGIFY( LPF ), LPF )
    .value( STRINGIFY( LSTMT ), LSTMT )
    .value( STRINGIFY( LTOK ), LTOK )
    .value( STRINGIFY( LALRC ), LALRC )
    .value( STRINGIFY( LCMTIND ), LCMTIND )
    .value( STRINGIFY( LINDLINE ), LINDLINE )
    .value( STRINGIFY( LSIB ), LSIB )
    .value( STRINGIFY( LRETURN ), LRETURN )
    .value( STRINGIFY( LBRDEL ), LBRDEL )
    .value( STRINGIFY( LFCN ), LFCN )
    .value( STRINGIFY( LFCNP ), LFCNP )
    .value( STRINGIFY( LPCU ), LPCU )
    .value( STRINGIFY( LDYNKW ), LDYNKW )
    .value( STRINGIFY( LOUTIND ), LOUTIND )
    .value( STRINGIFY( LBCSAFTER ), LBCSAFTER )
    .value( STRINGIFY( LBCSPOP ), LBCSPOP )
    .value( STRINGIFY( LBCSPUSH ), LBCSPUSH )
    .value( STRINGIFY( LBCSSWAP ), LBCSSWAP )
    .value( STRINGIFY( LFTOR ), LFTOR )
    .value( STRINGIFY( LAS ), LAS )
    .value( STRINGIFY( LPPIS ), LPPIS )
    .value( STRINGIFY( LTYPEDEF ), LTYPEDEF )
    .value( STRINGIFY( LVARDEF ), LVARDEF )
    .value( STRINGIFY( LDEFVAL ), LDEFVAL )
    .value( STRINGIFY( LPVSEMI ), LPVSEMI )
    .value( STRINGIFY( LPFUNC ), LPFUNC )
    .value( STRINGIFY( LSPLIT ), LSPLIT )
    .value( STRINGIFY( LFTYPE ), LFTYPE )
    .value( STRINGIFY( LTEMPL ), LTEMPL )
    .value( STRINGIFY( LPARADD ), LPARADD )
    .value( STRINGIFY( LPARADD2 ), LPARADD2 )
    .value( STRINGIFY( LBLANKD ), LBLANKD )
    .value( STRINGIFY( LTEMPFUNC ), LTEMPFUNC )
    .value( STRINGIFY( LSCANSEMI ), LSCANSEMI )
    .value( STRINGIFY( LDELSEMI ), LDELSEMI )
    .value( STRINGIFY( LFPARAM ), LFPARAM )
    .value( STRINGIFY( LNL1LINE ), LNL1LINE )
    .value( STRINGIFY( LPFCHK ), LPFCHK )
    .value( STRINGIFY( LAVDB ), LAVDB )
    .value( STRINGIFY( LSORT ), LSORT )
    .value( STRINGIFY( LSPACE ), LSPACE )
    .value( STRINGIFY( LALIGN ), LALIGN )
    .value( STRINGIFY( LALAGAIN ), LALAGAIN )
    .value( STRINGIFY( LOPERATOR ), LOPERATOR )
    .value( STRINGIFY( LASFCP ), LASFCP )
    .value( STRINGIFY( LINDLINED ), LINDLINED )
    .value( STRINGIFY( LBCTRL ), LBCTRL )
    .value( STRINGIFY( LRMRETURN ), LRMRETURN )
    .value( STRINGIFY( LPPIF ), LPPIF )
    .value( STRINGIFY( LMCB ), LMCB )
    .value( STRINGIFY( LBRCH ), LBRCH )
    .value( STRINGIFY( LFCNR ), LFCNR )
    .value( STRINGIFY( LOCCLASS ), LOCCLASS )
    .value( STRINGIFY( LOCMSG ), LOCMSG )
    .value( STRINGIFY( LBLANK ), LBLANK )
    .value( STRINGIFY( LOBJCWORD ), LOBJCWORD )
    .value( STRINGIFY( LCHANGE ), LCHANGE )
    .value( STRINGIFY( LCONTTEXT ), LCONTTEXT )
    .value( STRINGIFY( LANNOT ), LANNOT )
    .value( STRINGIFY( LOCBLK ), LOCBLK )
    .value( STRINGIFY( LFLPAREN ), LFLPAREN )
    .value( STRINGIFY( LOCMSGD ), LOCMSGD )
    .value( STRINGIFY( LINDENTAG ), LINDENTAG )
    .value( STRINGIFY( LNFD ), LNFD )
    .value( STRINGIFY( LJDBI ), LJDBI )
    .value( STRINGIFY( LSETPAR ), LSETPAR )
    .value( STRINGIFY( LSETTYP ), LSETTYP )
    .value( STRINGIFY( LSETFLG ), LSETFLG )
    .value( STRINGIFY( LNLFUNCT ), LNLFUNCT )
    .value( STRINGIFY( LCHUNK ), LCHUNK )
    .value( STRINGIFY( LGUY98 ), LGUY98 )
    .value( STRINGIFY( LGUY ), LGUY );

    enum_< lang_flags >( STRINGIFY( lang_flags ) )
    .value( STRINGIFY( LANG_C_ ), lang_flags::LANG_C_ )
    .value( STRINGIFY( LANG_CPP_ ), lang_flags::LANG_CPP_ )
    .value( STRINGIFY( LANG_D_ ), lang_flags::LANG_D_ )
    .value( STRINGIFY( LANG_CS_ ), lang_flags::LANG_CS_ )
    .value( STRINGIFY( LANG_VALA_ ), lang_flags::LANG_VALA_ )
    .value( STRINGIFY( LANG_JAVA_ ), lang_flags::LANG_JAVA_ )
    .value( STRINGIFY( LANG_PAWN_ ), lang_flags::LANG_PAWN_ )
    .value( STRINGIFY( LANG_OC_ ), lang_flags::LANG_OC_ )
    .value( STRINGIFY( LANG_OCPP_ ), lang_flags::LANG_OCPP_ )
    .value( STRINGIFY( LANG_ECMA_ ), lang_flags::LANG_ECMA_ );
//TODO ???? see language_names[] uncrustify.cpp, what about flags ?
//    .value( STRINGIFY( LANG_ALLC_ ), lang_flags::LANG_ALLC_ )
//    .value( STRINGIFY( LANG_ALL_ ), lang_flags::LANG_ALL_ )
}

#endif
