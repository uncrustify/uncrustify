/*
 * uncrustify_emscripten.cpp - JavaScript Emscripten binding interface
 *
 *  Created on: May 8, 2016
 *      Author: Daniel Chumak
 *
 * INTERFACE:
 * ============================================================================
 * unsure about these:
 *   --check       TODO ???
 *   --decode      TODO ???
 *   --detect      TODO needs uncrustify start and end which both are static
 *
 *
 * will not be included:
 * ----------------------------------------------------------------------------
 *   -t ( define via multiple --type )
 *   -d ( define via multiple --define )
 *   --assume ( no files available to guess the lang. based on the filename ending )
 *   --files ( no batch processing will be available )
 *   --prefix
 *   --suffix
 *   --assume
 *   --no-backup
 *   --replace
 *   --mtime
 *   --universalindent
 *   -help, -h, --usage, -?
 *
 *
 * done:
 * ----------------------------------------------------------------------------
 *   --update-config ( use show_config() )
 *   --update-config-with-doc ( show_config( bool withDoc = true ) )
 *   --version, -v ( use get_version() )
 *   --log, -L ( use log_type_enable( log_sev_t sev, bool value ) )
 *   -q ( use quiet() )
 *   --config, -c ( use loadConfig( string _cfg ) )
 *   --file, -f ( use uncrustify( string _file ) )
 *   --show ( use log_type_show_name( bool ) )
 *   --frag ( use uncrustify( string _file, bool frag = true ) )
 *   --type ( use add_keyword( string _type, c_token_t type ) )
 *   -l ( use uncrustify() )
 *   --parsed, -p  ( use debug() )
 */

#ifdef EMSCRIPTEN

#include "keywords.h"
#include "log_levels.h"
#include "logger.h"
#include "option.h"
#include "options.h"
#include "output.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "uncrustify_version.h"
#include "unicode.h"

#include <iostream>
#include <map>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace std;
using namespace emscripten;
using namespace uncrustify;


/**
 * Loads options from a file represented as a single char array.
 * Modifies: input char array, cpd.line_number
 * Expects: \0 terminated char array
 *
 * @param configString char array that holds the whole config
 * @return EXIT_SUCCESS on success
 */
int load_option_fileChar(char *configString)
{
   char *delimPos       = &configString[0];
   char *subStringStart = &configString[0];

   cpd.line_number = 0;

   // TODO: handle compat_level
   int compat_level = 0;

   while (true)
   {
      delimPos = strchr(delimPos, '\n');

      if (delimPos == nullptr)
      {
         break;
      }
      // replaces \n with \0 -> string including multiple terminated substrings
      *delimPos = '\0';


      process_option_line(subStringStart, "", compat_level);

      delimPos++;
      subStringStart = delimPos;
   }
   //get last line, expectation: ends with \0
   process_option_line(subStringStart, "", compat_level);

   return(EXIT_SUCCESS);
}


/**
 * adds a new keyword to Uncrustify's dynamic keyword map (dkwm, keywords.cpp)
 *
 * @param tag:  keyword that is going to be added
 * @param type: type of the keyword
 */
void _add_keyword(string tag, c_token_t type)
{
   if (tag.empty())
   {
      LOG_FMT(LERR, "%s: input string is empty\n", __func__);
      return;
   }
   add_keyword(tag, type);
}


//! clears Uncrustify's dynamic keyword map (dkwm, keywords.cpp)
void clear_keywords()
{
   clear_keyword_file();
}


/**
 * Show or hide the severity prefix "<1>"
 *
 * @param b: true=show, false=hide
 */
void show_log_type(bool b)
{
   log_show_sev(b);
}


//! returns the UNCRUSTIFY_VERSION string
string get_version()
{
   return(UNCRUSTIFY_VERSION);
}


//! disables all logging messages
void set_quiet()
{
   // set empty mask
   log_set_mask({});
}


/**
 * resets value of an option to its default
 *
 * @param name:  name of the option
 * @return options enum value of the found option or -1 if option was not found
 */
int reset_option(string name)
{
   if (name.empty())
   {
      LOG_FMT(LERR, "%s: name string is empty\n", __func__);
      return(-1);
   }
   const auto option = find_option(name.c_str());

   if (option == nullptr)
   {
      LOG_FMT(LERR, "Option %s not found\n", name.c_str());
      return(-1);
   }
   option->reset();
   return(0);
}


/**
 * sets value of an option
 *
 * @param name:  name of the option
 * @param value: value that is going to be set
 * @return options enum value of the found option or -1 if option was not found
 */
int set_option(string name, string value)
{
   if (name.empty())
   {
      LOG_FMT(LERR, "%s: name string is empty\n", __func__);
      return(-1);
   }

   if (value.empty())
   {
      LOG_FMT(LERR, "%s: value string is empty\n", __func__);
      return(-1);
   }
   const auto option = find_option(name.c_str());

   if (option == nullptr)
   {
      LOG_FMT(LERR, "Option %s not found\n", name.c_str());
      return(-1);
   }

   if (!option->read(value.c_str()))
   {
      LOG_FMT(
         LERR,
         "Failed to set value %s for option: %s of type: %s\n",
         name.c_str(),
         value.c_str(),
         to_string(option->type())
         );
      return(-1);
   }
   return(0);
}


/**
 * returns value of an option
 *
 * @param name: name of the option
 * @return currently set value of the option
 */
string get_option(string name)
{
   if (name.empty())
   {
      LOG_FMT(LERR, "%s: input string is empty\n", __func__);
      return("");
   }
   const auto option = find_option(name.c_str());

   if (option == nullptr)
   {
      LOG_FMT(LWARN, "Option %s not found\n", name.c_str());
      return("");
   }
   return(option->str());
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
string show_config(bool withDoc, bool only_not_default)
{
   char   *buf;
   size_t len;

   FILE   *stream = open_memstream(&buf, &len);

   if (stream == nullptr)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      fflush(stream);
      fclose(stream);
      free(buf);
      return("");
   }
   save_option_file(stream, withDoc, only_not_default);

   fflush(stream);
   fclose(stream);

   string out(buf);

   free(buf);

   return(out);
}


/**
 * returns the config file string with all options based on the current configuration
 *
 * @param withDoc: false= without documentation, true=with documentation text lines
 * @return returns the config file string with all options based on the current configuration
 */
string show_config(bool withDoc)
{
   return(show_config(withDoc, false));
}


//!returns the config file string with all options and without documentation based on the current configuration
string show_config()
{
   return(show_config(false, false));
}


std::vector<OptionGroup *> get_groups()
{
   std::vector<OptionGroup *> groups;

   groups.reserve(5);

   for (size_t i = 0; true; ++i)
   {
      OptionGroup *group = get_option_group(i);

      if (!group)
      {
         break;
      }
      groups.push_back(group);
   }

   return(groups);
}


std::vector<GenericOption *> get_options()
{
   std::vector<GenericOption *> options;

   options.reserve(get_option_count());

   for (size_t i = 0; true; ++i)
   {
      OptionGroup *group = get_option_group(i);

      if (!group)
      {
         break;
      }
      options.insert(
         end(options),
         begin(group->options),
         end(group->options)
         );
   }

   return(options);
}


//! resets all options to their default values
void reset_options()
{
   auto options = get_options();

   for (auto *option : options)
   {
      option->reset();
   }
}


/**
 * initializes the current libUncrustify instance,
 * used only for emscripten binding here and will be automatically called while
 * module initialization
 */
void _initialize()
{
   register_options();
   log_init(stdout);

   LOG_FMT(LSYS, "Initialized libUncrustify - " UNCRUSTIFY_VERSION "\n");
}


//! destroys the current libUncrustify instance
void destruct()
{
   clear_keyword_file();
}


/**
 * reads option file string, sets the defined options
 *
 * @return returns EXIT_SUCCESS on success
 */
int _loadConfig(intptr_t _cfg)
{
   // reset everything in case a config was loaded previously
   clear_keyword_file();
   reset_options();

   // embind complains about char* so we use an int to get the pointer and cast
   // it, memory management is done in /emscripten/postfix_module.js
   char *cfg = reinterpret_cast<char *>(_cfg);

   if (load_option_fileChar(cfg) != EXIT_SUCCESS)
   {
      LOG_FMT(LERR, "unable to load the config\n");
      return(EXIT_FAILURE);
   }
   // This relies on cpd.filename being the config file name
   load_header_files();

   LOG_FMT(LSYS, "finished loading config\n");
   return(EXIT_SUCCESS);
}


/**
 * format string
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 * @param frag: true=fragmented code input, false=unfragmented code input
 * @param defer: true=do not perform cleanup of Uncrustify structures
 *
 * @return pointer to the formatted file char* string
 */
intptr_t _uncrustify(intptr_t _file, lang_flag_e langIDX, bool frag, bool defer)
{
   // Problem: uncrustify originally is not a lib and uses global vars such as
   // cpd.error_count for the whole program execution
   // to know if errors occurred during the formating step we reset this var here
   cpd.error_count = 0;
   cpd.filename    = "stdin";
   cpd.frag        = frag;

   if (langIDX == 0)   // 0 == undefined
   {
      LOG_FMT(LWARN, "language of input file not defined, C++ will be assumed\n");
      cpd.lang_flags = LANG_CPP;
   }
   else
   {
      cpd.lang_flags = langIDX;
   }
   // embind complains about char* so we use an intptr_t to get the pointer and
   // cast it, memory management is done in /emscripten/postfix_module.js
   char     *file = reinterpret_cast<char *>(_file);

   file_mem fm;

   fm.raw.clear();
   fm.data.clear();
   fm.enc = char_encoding_e::e_ASCII;
   fm.raw = vector<UINT8>();

   char c;

   for (auto idx = 0; (c = file[idx]) != 0; ++idx)
   {
      fm.raw.push_back(c);
   }

   if (!decode_unicode(fm.raw, fm.data, fm.enc, fm.bom))
   {
      LOG_FMT(LERR, "Failed to read code\n");
      return(0);
   }
   // Done reading from stdin
   LOG_FMT(LSYS, "Parsing: %d bytes (%d chars) from stdin as language %s\n",
           (int)fm.raw.size(), (int)fm.data.size(),
           language_name_from_flags(cpd.lang_flags));


   char   *buf = nullptr;
   size_t len  = 0;

   // uncrustify uses FILE instead of streams for its outputs
   // to redirect FILE writes into a char* open_memstream is used
   // windows lacks open_memstream, only UNIX/BSD is supported
   // apparently emscripten has its own implementation, if that is not working
   // see: stackoverflow.com/questions/10305095#answer-10341073
   FILE *stream = open_memstream(&buf, &len);

   if (stream == nullptr)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      return(0);
   }
   // TODO One way to implement the --parsed, -p functionality would
   // be to let the uncrustify_file function run, throw away the formated
   // output and return the debug as a string. For this uncrustify_file would
   // need to accept a stream, FILE or a char array pointer in which the output
   // will be stored.
   // Another option would be to check, inside the uncrustify_file function,
   // if the current filename string matches stdout or stderr and use those as
   // output locations. This is the easier fix but the debug info in the
   // browsers console is littered with other unneeded text.
   // Finally, the ugliest solution, would be also possible to re-route
   // either stdout or stderr inside the Module var of emscripten to a js
   // function which passes the debug output into a dedicated output js target.
   // This therefore would introduce the dependency on the user to always have
   // the output js target available.
   uncrustify_file(fm, stream, nullptr, defer);

   fflush(stream);
   fclose(stream);

   if (cpd.error_count != 0)
   {
      LOG_FMT(LWARN, "%d errors occurred during formating\n", cpd.error_count);
   }

   if (len == 0)
   {
      return(0);
   }
   // buf is deleted inside js code
   return(reinterpret_cast<intptr_t>(buf));
} // uncrustify


/**
 * format string
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 * @param frag: true=fragmented code input, false=unfragmented code input
 *
 * @return pointer to the formatted file char* string
 */
intptr_t _uncrustify(intptr_t file, lang_flag_e langIDX, bool frag)
{
   return(_uncrustify(file, langIDX, frag, false));
}


/**
 * format string, assume unfragmented code input
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 *
 * @return pointer to the formatted file char* string
 */
intptr_t _uncrustify(intptr_t file, lang_flag_e langIDX)
{
   return(_uncrustify(file, langIDX, false, false));
}


/**
 * generate debug output
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 * @param frag: true=fragmented code input, false=unfragmented code input
 *
 * @return pointer to the debug file char* string
 */
intptr_t _debug(intptr_t _file, lang_flag_e langIDX, bool frag)
{
   auto formatted_str_ptr = _uncrustify(_file, langIDX, frag, true);
   char *formatted_str    = reinterpret_cast<char *>(formatted_str_ptr);

   // Lazy solution: Throw away the formated file output.
   // Maybe later add option to return both formatted file string and debug
   // file string together ... somehow.
   free(formatted_str);

   char   *buf    = nullptr;
   size_t len     = 0;
   FILE   *stream = open_memstream(&buf, &len);

   if (stream == nullptr)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      return(0);
   }
   output_parsed(stream);
   fflush(stream);
   fclose(stream);

   // start deferred _uncrustify cleanup
   uncrustify_end();

   if (len == 0)
   {
      return(0);
   }
   // buf is deleted inside js code
   return(reinterpret_cast<intptr_t>(buf));
} // uncrustify


/**
 * generate debug output, assume unfragmented code input
 *
 * @param file: pointer to the file char* string that is going to be formatted
 * @param langIDX: specifies in which language the input file is written
 *
 * @return pointer to the debug file char* string
 */
intptr_t _debug(intptr_t _file, lang_flag_e langIDX)
{
   return(_debug(_file, langIDX, false));
}


EMSCRIPTEN_BINDINGS(MainModule)
{
   // region enum bindings
   enum_<option_type_e>("OptionType")
      .value("BOOL", option_type_e::BOOL)
      .value("IARF", option_type_e::IARF)
      .value("LINEEND", option_type_e::LINEEND)
      .value("TOKENPOS", option_type_e::TOKENPOS)
      .value("NUM", option_type_e::NUM)
      .value("UNUM", option_type_e::UNUM)
      .value("STRING", option_type_e::STRING);

   enum_<iarf_e>("IARF")
      .value("IGNORE", iarf_e::IGNORE)
      .value("ADD", iarf_e::ADD)
      .value("REMOVE", iarf_e::REMOVE)
      .value("FORCE", iarf_e::FORCE);

   enum_<line_end_e>("LineEnd")
      .value("LF", line_end_e::LF)
      .value("CRLF", line_end_e::CRLF)
      .value("CR", line_end_e::CR)
      .value("AUTO", line_end_e::AUTO);

   enum_<token_pos_e>("TokenPos")
      .value("IGNORE", token_pos_e::IGNORE)
      .value("BREAK", token_pos_e::BREAK)
      .value("FORCE", token_pos_e::FORCE)
      .value("LEAD", token_pos_e::LEAD)
      .value("TRAIL", token_pos_e::TRAIL)
      .value("JOIN", token_pos_e::JOIN)
      .value("LEAD_BREAK", token_pos_e::LEAD_BREAK)
      .value("LEAD_FORCE", token_pos_e::LEAD_FORCE)
      .value("TRAIL_BREAK", token_pos_e::TRAIL_BREAK)
      .value("TRAIL_FORCE", token_pos_e::TRAIL_FORCE);

   enum_<log_sev_t>("LogType")
      .value("SYS", log_sev_t::LSYS)
      .value("ERR", log_sev_t::LERR)
      .value("WARN", log_sev_t::LWARN)
      .value("NOTE", log_sev_t::LNOTE)
      .value("INFO", log_sev_t::LINFO)
      .value("DATA", log_sev_t::LDATA)
      .value("FILELIST", log_sev_t::LFILELIST)
      .value("LINEENDS", log_sev_t::LLINEENDS)
      .value("CASTS", log_sev_t::LCASTS)
      .value("ALBR", log_sev_t::LALBR)
      .value("ALTD", log_sev_t::LALTD)
      .value("ALPP", log_sev_t::LALPP)
      .value("ALPROTO", log_sev_t::LALPROTO)
      .value("ALNLC", log_sev_t::LALNLC)
      .value("ALTC", log_sev_t::LALTC)
      .value("ALADD", log_sev_t::LALADD)
      .value("ALASS", log_sev_t::LALASS)
      .value("FVD", log_sev_t::LFVD)
      .value("FVD2", log_sev_t::LFVD2)
      .value("INDENT", log_sev_t::LINDENT)
      .value("INDENT2", log_sev_t::LINDENT2)
      .value("INDPSE", log_sev_t::LINDPSE)
      .value("INDPC", log_sev_t::LINDPC)
      .value("NEWLINE", log_sev_t::LNEWLINE)
      .value("PF", log_sev_t::LPF)
      .value("STMT", log_sev_t::LSTMT)
      .value("TOK", log_sev_t::LTOK)
      .value("ALRC", log_sev_t::LALRC)
      .value("CMTIND", log_sev_t::LCMTIND)
      .value("INDLINE", log_sev_t::LINDLINE)
      .value("SIB", log_sev_t::LSIB)
      .value("RETURN", log_sev_t::LRETURN)
      .value("BRDEL", log_sev_t::LBRDEL)
      .value("FCN", log_sev_t::LFCN)
      .value("FCNP", log_sev_t::LFCNP)
      .value("PCU", log_sev_t::LPCU)
      .value("DYNKW", log_sev_t::LDYNKW)
      .value("OUTIND", log_sev_t::LOUTIND)
      .value("BCSAFTER", log_sev_t::LBCSAFTER)
      .value("BCSPOP", log_sev_t::LBCSPOP)
      .value("BCSPUSH", log_sev_t::LBCSPUSH)
      .value("BCSSWAP", log_sev_t::LBCSSWAP)
      .value("FTOR", log_sev_t::LFTOR)
      .value("AS", log_sev_t::LAS)
      .value("PPIS", log_sev_t::LPPIS)
      .value("TYPEDEF", log_sev_t::LTYPEDEF)
      .value("VARDEF", log_sev_t::LVARDEF)
      .value("DEFVAL", log_sev_t::LDEFVAL)
      .value("PVSEMI", log_sev_t::LPVSEMI)
      .value("PFUNC", log_sev_t::LPFUNC)
      .value("SPLIT", log_sev_t::LSPLIT)
      .value("FTYPE", log_sev_t::LFTYPE)
      .value("TEMPL", log_sev_t::LTEMPL)
      .value("PARADD", log_sev_t::LPARADD)
      .value("PARADD2", log_sev_t::LPARADD2)
      .value("BLANKD", log_sev_t::LBLANKD)
      .value("TEMPFUNC", log_sev_t::LTEMPFUNC)
      .value("SCANSEMI", log_sev_t::LSCANSEMI)
      .value("DELSEMI", log_sev_t::LDELSEMI)
      .value("FPARAM", log_sev_t::LFPARAM)
      .value("NL1LINE", log_sev_t::LNL1LINE)
      .value("PFCHK", log_sev_t::LPFCHK)
      .value("AVDB", log_sev_t::LAVDB)
      .value("SORT", log_sev_t::LSORT)
      .value("SPACE", log_sev_t::LSPACE)
      .value("ALIGN", log_sev_t::LALIGN)
      .value("ALAGAIN", log_sev_t::LALAGAIN)
      .value("OPERATOR", log_sev_t::LOPERATOR)
      .value("ASFCP", log_sev_t::LASFCP)
      .value("INDLINED", log_sev_t::LINDLINED)
      .value("BCTRL", log_sev_t::LBCTRL)
      .value("RMRETURN", log_sev_t::LRMRETURN)
      .value("PPIF", log_sev_t::LPPIF)
      .value("MCB", log_sev_t::LMCB)
      .value("BRCH", log_sev_t::LBRCH)
      .value("FCNR", log_sev_t::LFCNR)
      .value("OCCLASS", log_sev_t::LOCCLASS)
      .value("OCMSG", log_sev_t::LOCMSG)
      .value("BLANK", log_sev_t::LBLANK)
      .value("OBJCWORD", log_sev_t::LOBJCWORD)
      .value("CHANGE", log_sev_t::LCHANGE)
      .value("CONTTEXT", log_sev_t::LCONTTEXT)
      .value("ANNOT", log_sev_t::LANNOT)
      .value("OCBLK", log_sev_t::LOCBLK)
      .value("FLPAREN", log_sev_t::LFLPAREN)
      .value("OCMSGD", log_sev_t::LOCMSGD)
      .value("INDENTAG", log_sev_t::LINDENTAG)
      .value("NFD", log_sev_t::LNFD)
      .value("JDBI", log_sev_t::LJDBI)
      .value("SETPAR", log_sev_t::LSETPAR)
      .value("SETTYP", log_sev_t::LSETTYP)
      .value("SETFLG", log_sev_t::LSETFLG)
      .value("NLFUNCT", log_sev_t::LNLFUNCT)
      .value("CHUNK", log_sev_t::LCHUNK)
      .value("GUY98", log_sev_t::LGUY98)
      .value("GUY", log_sev_t::LGUY);

   enum_<c_token_t>("TokenType")
      .value("NONE", c_token_t::CT_NONE)
      .value("EOF", c_token_t::CT_EOF)
      .value("UNKNOWN", c_token_t::CT_UNKNOWN)
      .value("JUNK", c_token_t::CT_JUNK)
      .value("WHITESPACE", c_token_t::CT_WHITESPACE)
      .value("SPACE", c_token_t::CT_SPACE)
      .value("NEWLINE", c_token_t::CT_NEWLINE)
      .value("NL_CONT", c_token_t::CT_NL_CONT)
      .value("COMMENT_CPP", c_token_t::CT_COMMENT_CPP)
      .value("COMMENT", c_token_t::CT_COMMENT)
      .value("COMMENT_MULTI", c_token_t::CT_COMMENT_MULTI)
      .value("COMMENT_EMBED", c_token_t::CT_COMMENT_EMBED)
      .value("COMMENT_START", c_token_t::CT_COMMENT_START)
      .value("COMMENT_END", c_token_t::CT_COMMENT_END)
      .value("COMMENT_WHOLE", c_token_t::CT_COMMENT_WHOLE)
      .value("COMMENT_ENDIF", c_token_t::CT_COMMENT_ENDIF)
      .value("IGNORED", c_token_t::CT_IGNORED)
      .value("WORD", c_token_t::CT_WORD)
      .value("NUMBER", c_token_t::CT_NUMBER)
      .value("NUMBER_FP", c_token_t::CT_NUMBER_FP)
      .value("STRING", c_token_t::CT_STRING)
      .value("STRING_MULTI", c_token_t::CT_STRING_MULTI)
      .value("IF", c_token_t::CT_IF)
      .value("ELSE", c_token_t::CT_ELSE)
      .value("ELSEIF", c_token_t::CT_ELSEIF)
      .value("FOR", c_token_t::CT_FOR)
      .value("WHILE", c_token_t::CT_WHILE)
      .value("WHILE_OF_DO", c_token_t::CT_WHILE_OF_DO)
      .value("SWITCH", c_token_t::CT_SWITCH)
      .value("CASE", c_token_t::CT_CASE)
      .value("DO", c_token_t::CT_DO)
      .value("SYNCHRONIZED", c_token_t::CT_SYNCHRONIZED)
      .value("VOLATILE", c_token_t::CT_VOLATILE)
      .value("TYPEDEF", c_token_t::CT_TYPEDEF)
      .value("STRUCT", c_token_t::CT_STRUCT)
      .value("ENUM", c_token_t::CT_ENUM)
      .value("ENUM_CLASS", c_token_t::CT_ENUM_CLASS)
      .value("SIZEOF", c_token_t::CT_SIZEOF)
      .value("DECLTYPE", c_token_t::CT_DECLTYPE)
      .value("RETURN", c_token_t::CT_RETURN)
      .value("BREAK", c_token_t::CT_BREAK)
      .value("UNION", c_token_t::CT_UNION)
      .value("GOTO", c_token_t::CT_GOTO)
      .value("CONTINUE", c_token_t::CT_CONTINUE)
      .value("C_CAST", c_token_t::CT_C_CAST)
      .value("CPP_CAST", c_token_t::CT_CPP_CAST)
      .value("D_CAST", c_token_t::CT_D_CAST)
      .value("TYPE_CAST", c_token_t::CT_TYPE_CAST)
      .value("TYPENAME", c_token_t::CT_TYPENAME)
      .value("TEMPLATE", c_token_t::CT_TEMPLATE)
      .value("WHERE_SPEC", c_token_t::CT_WHERE_SPEC)
      .value("ASSIGN", c_token_t::CT_ASSIGN)
      .value("ASSIGN_NL", c_token_t::CT_ASSIGN_NL)
      .value("SASSIGN", c_token_t::CT_SASSIGN)
      .value("ASSIGN_DEFAULT_ARG", c_token_t::CT_ASSIGN_DEFAULT_ARG)
      .value("ASSIGN_FUNC_PROTO", c_token_t::CT_ASSIGN_FUNC_PROTO)
      .value("COMPARE", c_token_t::CT_COMPARE)
      .value("SCOMPARE", c_token_t::CT_SCOMPARE)
      .value("BOOL", c_token_t::CT_BOOL)
      .value("SBOOL", c_token_t::CT_SBOOL)
      .value("ARITH", c_token_t::CT_ARITH)
      .value("SARITH", c_token_t::CT_SARITH)
      .value("CARET", c_token_t::CT_CARET)
      .value("DEREF", c_token_t::CT_DEREF)
      .value("INCDEC_BEFORE", c_token_t::CT_INCDEC_BEFORE)
      .value("INCDEC_AFTER", c_token_t::CT_INCDEC_AFTER)
      .value("MEMBER", c_token_t::CT_MEMBER)
      .value("DC_MEMBER", c_token_t::CT_DC_MEMBER)
      .value("C99_MEMBER", c_token_t::CT_C99_MEMBER)
      .value("INV", c_token_t::CT_INV)
      .value("DESTRUCTOR", c_token_t::CT_DESTRUCTOR)
      .value("NOT", c_token_t::CT_NOT)
      .value("D_TEMPLATE", c_token_t::CT_D_TEMPLATE)
      .value("ADDR", c_token_t::CT_ADDR)
      .value("NEG", c_token_t::CT_NEG)
      .value("POS", c_token_t::CT_POS)
      .value("STAR", c_token_t::CT_STAR)
      .value("PLUS", c_token_t::CT_PLUS)
      .value("MINUS", c_token_t::CT_MINUS)
      .value("AMP", c_token_t::CT_AMP)
      .value("BYREF", c_token_t::CT_BYREF)
      .value("POUND", c_token_t::CT_POUND)
      .value("PREPROC", c_token_t::CT_PREPROC)
      .value("PREPROC_INDENT", c_token_t::CT_PREPROC_INDENT)
      .value("PREPROC_BODY", c_token_t::CT_PREPROC_BODY)
      .value("PP", c_token_t::CT_PP)
      .value("ELLIPSIS", c_token_t::CT_ELLIPSIS)
      .value("RANGE", c_token_t::CT_RANGE)
      .value("NULLCOND", c_token_t::CT_NULLCOND)
      .value("SEMICOLON", c_token_t::CT_SEMICOLON)
      .value("VSEMICOLON", c_token_t::CT_VSEMICOLON)
      .value("COLON", c_token_t::CT_COLON)
      .value("ASM_COLON", c_token_t::CT_ASM_COLON)
      .value("CASE_COLON", c_token_t::CT_CASE_COLON)
      .value("CLASS_COLON", c_token_t::CT_CLASS_COLON)
      .value("CONSTR_COLON", c_token_t::CT_CONSTR_COLON)
      .value("D_ARRAY_COLON", c_token_t::CT_D_ARRAY_COLON)
      .value("COND_COLON", c_token_t::CT_COND_COLON)
      .value("WHERE_COLON", c_token_t::CT_WHERE_COLON)
      .value("QUESTION", c_token_t::CT_QUESTION)
      .value("COMMA", c_token_t::CT_COMMA)
      .value("ASM", c_token_t::CT_ASM)
      .value("ATTRIBUTE", c_token_t::CT_ATTRIBUTE)
      .value("AUTORELEASEPOOL", c_token_t::CT_AUTORELEASEPOOL)
      .value("OC_AVAILABLE", c_token_t::CT_OC_AVAILABLE)
      .value("OC_AVAILABLE_VALUE", c_token_t::CT_OC_AVAILABLE_VALUE)
      .value("CATCH", c_token_t::CT_CATCH)
      .value("WHEN", c_token_t::CT_WHEN)
      .value("WHERE", c_token_t::CT_WHERE)
      .value("CLASS", c_token_t::CT_CLASS)
      .value("DELETE", c_token_t::CT_DELETE)
      .value("EXPORT", c_token_t::CT_EXPORT)
      .value("FRIEND", c_token_t::CT_FRIEND)
      .value("NAMESPACE", c_token_t::CT_NAMESPACE)
      .value("PACKAGE", c_token_t::CT_PACKAGE)
      .value("NEW", c_token_t::CT_NEW)
      .value("OPERATOR", c_token_t::CT_OPERATOR)
      .value("OPERATOR_VAL", c_token_t::CT_OPERATOR_VAL)
      .value("ASSIGN_OPERATOR", c_token_t::CT_ASSIGN_OPERATOR)
      .value("ACCESS", c_token_t::CT_ACCESS)
      .value("ACCESS_COLON", c_token_t::CT_ACCESS_COLON)
      .value("THROW", c_token_t::CT_THROW)
      .value("NOEXCEPT", c_token_t::CT_NOEXCEPT)
      .value("TRY", c_token_t::CT_TRY)
      .value("BRACED_INIT_LIST", c_token_t::CT_BRACED_INIT_LIST)
      .value("USING", c_token_t::CT_USING)
      .value("USING_STMT", c_token_t::CT_USING_STMT)
      .value("USING_ALIAS", c_token_t::CT_USING_ALIAS)
      .value("D_WITH", c_token_t::CT_D_WITH)
      .value("D_MODULE", c_token_t::CT_D_MODULE)
      .value("SUPER", c_token_t::CT_SUPER)
      .value("DELEGATE", c_token_t::CT_DELEGATE)
      .value("BODY", c_token_t::CT_BODY)
      .value("DEBUG", c_token_t::CT_DEBUG)
      .value("DEBUGGER", c_token_t::CT_DEBUGGER)
      .value("INVARIANT", c_token_t::CT_INVARIANT)
      .value("UNITTEST", c_token_t::CT_UNITTEST)
      .value("UNSAFE", c_token_t::CT_UNSAFE)
      .value("FINALLY", c_token_t::CT_FINALLY)
      .value("FIXED", c_token_t::CT_FIXED)
      .value("IMPORT", c_token_t::CT_IMPORT)
      .value("D_SCOPE", c_token_t::CT_D_SCOPE)
      .value("D_SCOPE_IF", c_token_t::CT_D_SCOPE_IF)
      .value("LAZY", c_token_t::CT_LAZY)
      .value("D_MACRO", c_token_t::CT_D_MACRO)
      .value("D_VERSION", c_token_t::CT_D_VERSION)
      .value("D_VERSION_IF", c_token_t::CT_D_VERSION_IF)
      .value("PAREN_OPEN", c_token_t::CT_PAREN_OPEN)
      .value("PAREN_CLOSE", c_token_t::CT_PAREN_CLOSE)
      .value("ANGLE_OPEN", c_token_t::CT_ANGLE_OPEN)
      .value("ANGLE_CLOSE", c_token_t::CT_ANGLE_CLOSE)
      .value("SPAREN_OPEN", c_token_t::CT_SPAREN_OPEN)
      .value("SPAREN_CLOSE", c_token_t::CT_SPAREN_CLOSE)
      .value("FPAREN_OPEN", c_token_t::CT_FPAREN_OPEN)
      .value("FPAREN_CLOSE", c_token_t::CT_FPAREN_CLOSE)
      .value("TPAREN_OPEN", c_token_t::CT_TPAREN_OPEN)
      .value("TPAREN_CLOSE", c_token_t::CT_TPAREN_CLOSE)
      .value("BRACE_OPEN", c_token_t::CT_BRACE_OPEN)
      .value("BRACE_CLOSE", c_token_t::CT_BRACE_CLOSE)
      .value("VBRACE_OPEN", c_token_t::CT_VBRACE_OPEN)
      .value("VBRACE_CLOSE", c_token_t::CT_VBRACE_CLOSE)
      .value("SQUARE_OPEN", c_token_t::CT_SQUARE_OPEN)
      .value("SQUARE_CLOSE", c_token_t::CT_SQUARE_CLOSE)
      .value("TSQUARE", c_token_t::CT_TSQUARE)
      .value("MACRO_OPEN", c_token_t::CT_MACRO_OPEN)
      .value("MACRO_CLOSE", c_token_t::CT_MACRO_CLOSE)
      .value("MACRO_ELSE", c_token_t::CT_MACRO_ELSE)
      .value("LABEL", c_token_t::CT_LABEL)
      .value("LABEL_COLON", c_token_t::CT_LABEL_COLON)
      .value("FUNCTION", c_token_t::CT_FUNCTION)
      .value("FUNC_CALL", c_token_t::CT_FUNC_CALL)
      .value("FUNC_CALL_USER", c_token_t::CT_FUNC_CALL_USER)
      .value("FUNC_DEF", c_token_t::CT_FUNC_DEF)
      .value("FUNC_TYPE", c_token_t::CT_FUNC_TYPE)
      .value("FUNC_VAR", c_token_t::CT_FUNC_VAR)
      .value("FUNC_PROTO", c_token_t::CT_FUNC_PROTO)
      .value("FUNC_START", c_token_t::CT_FUNC_START)
      .value("FUNC_CLASS_DEF", c_token_t::CT_FUNC_CLASS_DEF)
      .value("FUNC_CLASS_PROTO", c_token_t::CT_FUNC_CLASS_PROTO)
      .value("FUNC_CTOR_VAR", c_token_t::CT_FUNC_CTOR_VAR)
      .value("FUNC_WRAP", c_token_t::CT_FUNC_WRAP)
      .value("PROTO_WRAP", c_token_t::CT_PROTO_WRAP)
      .value("MACRO_FUNC", c_token_t::CT_MACRO_FUNC)
      .value("MACRO", c_token_t::CT_MACRO)
      .value("QUALIFIER", c_token_t::CT_QUALIFIER)
      .value("EXTERN", c_token_t::CT_EXTERN)
      .value("DECLSPEC", c_token_t::CT_DECLSPEC)
      .value("ALIGN", c_token_t::CT_ALIGN)
      .value("TYPE", c_token_t::CT_TYPE)
      .value("PTR_TYPE", c_token_t::CT_PTR_TYPE)
      .value("TYPE_WRAP", c_token_t::CT_TYPE_WRAP)
      .value("CPP_LAMBDA", c_token_t::CT_CPP_LAMBDA)
      .value("CPP_LAMBDA_RET", c_token_t::CT_CPP_LAMBDA_RET)
      .value("TRAILING_RET", c_token_t::CT_TRAILING_RET)
      .value("BIT_COLON", c_token_t::CT_BIT_COLON)
      .value("OC_DYNAMIC", c_token_t::CT_OC_DYNAMIC)
      .value("OC_END", c_token_t::CT_OC_END)
      .value("OC_IMPL", c_token_t::CT_OC_IMPL)
      .value("OC_INTF", c_token_t::CT_OC_INTF)
      .value("OC_PROTOCOL", c_token_t::CT_OC_PROTOCOL)
      .value("OC_PROTO_LIST", c_token_t::CT_OC_PROTO_LIST)
      .value("OC_GENERIC_SPEC", c_token_t::CT_OC_GENERIC_SPEC)
      .value("OC_PROPERTY", c_token_t::CT_OC_PROPERTY)
      .value("OC_CLASS", c_token_t::CT_OC_CLASS)
      .value("OC_CLASS_EXT", c_token_t::CT_OC_CLASS_EXT)
      .value("OC_CATEGORY", c_token_t::CT_OC_CATEGORY)
      .value("OC_SCOPE", c_token_t::CT_OC_SCOPE)
      .value("OC_MSG", c_token_t::CT_OC_MSG)
      .value("OC_MSG_CLASS", c_token_t::CT_OC_MSG_CLASS)
      .value("OC_MSG_FUNC", c_token_t::CT_OC_MSG_FUNC)
      .value("OC_MSG_NAME", c_token_t::CT_OC_MSG_NAME)
      .value("OC_MSG_SPEC", c_token_t::CT_OC_MSG_SPEC)
      .value("OC_MSG_DECL", c_token_t::CT_OC_MSG_DECL)
      .value("OC_RTYPE", c_token_t::CT_OC_RTYPE)
      .value("OC_ATYPE", c_token_t::CT_OC_ATYPE)
      .value("OC_COLON", c_token_t::CT_OC_COLON)
      .value("OC_DICT_COLON", c_token_t::CT_OC_DICT_COLON)
      .value("OC_SEL", c_token_t::CT_OC_SEL)
      .value("OC_SEL_NAME", c_token_t::CT_OC_SEL_NAME)
      .value("OC_BLOCK", c_token_t::CT_OC_BLOCK)
      .value("OC_BLOCK_ARG", c_token_t::CT_OC_BLOCK_ARG)
      .value("OC_BLOCK_TYPE", c_token_t::CT_OC_BLOCK_TYPE)
      .value("OC_BLOCK_EXPR", c_token_t::CT_OC_BLOCK_EXPR)
      .value("OC_BLOCK_CARET", c_token_t::CT_OC_BLOCK_CARET)
      .value("OC_AT", c_token_t::CT_OC_AT)
      .value("OC_PROPERTY_ATTR", c_token_t::CT_OC_PROPERTY_ATTR)
      .value("PP_DEFINE", c_token_t::CT_PP_DEFINE)
      .value("PP_DEFINED", c_token_t::CT_PP_DEFINED)
      .value("PP_INCLUDE", c_token_t::CT_PP_INCLUDE)
      .value("PP_IF", c_token_t::CT_PP_IF)
      .value("PP_ELSE", c_token_t::CT_PP_ELSE)
      .value("PP_ENDIF", c_token_t::CT_PP_ENDIF)
      .value("PP_ASSERT", c_token_t::CT_PP_ASSERT)
      .value("PP_EMIT", c_token_t::CT_PP_EMIT)
      .value("PP_ENDINPUT", c_token_t::CT_PP_ENDINPUT)
      .value("PP_ERROR", c_token_t::CT_PP_ERROR)
      .value("PP_FILE", c_token_t::CT_PP_FILE)
      .value("PP_LINE", c_token_t::CT_PP_LINE)
      .value("PP_SECTION", c_token_t::CT_PP_SECTION)
      .value("PP_ASM", c_token_t::CT_PP_ASM)
      .value("PP_UNDEF", c_token_t::CT_PP_UNDEF)
      .value("PP_PROPERTY", c_token_t::CT_PP_PROPERTY)
      .value("PP_BODYCHUNK", c_token_t::CT_PP_BODYCHUNK)
      .value("PP_PRAGMA", c_token_t::CT_PP_PRAGMA)
      .value("PP_REGION", c_token_t::CT_PP_REGION)
      .value("PP_ENDREGION", c_token_t::CT_PP_ENDREGION)
      .value("PP_REGION_INDENT", c_token_t::CT_PP_REGION_INDENT)
      .value("PP_IF_INDENT", c_token_t::CT_PP_IF_INDENT)
      .value("PP_IGNORE", c_token_t::CT_PP_IGNORE)
      .value("PP_OTHER", c_token_t::CT_PP_OTHER)
      .value("CHAR", c_token_t::CT_CHAR)
      .value("DEFINED", c_token_t::CT_DEFINED)
      .value("FORWARD", c_token_t::CT_FORWARD)
      .value("NATIVE", c_token_t::CT_NATIVE)
      .value("STATE", c_token_t::CT_STATE)
      .value("STOCK", c_token_t::CT_STOCK)
      .value("TAGOF", c_token_t::CT_TAGOF)
      .value("DOT", c_token_t::CT_DOT)
      .value("TAG", c_token_t::CT_TAG)
      .value("TAG_COLON", c_token_t::CT_TAG_COLON)
      .value("LOCK", c_token_t::CT_LOCK)
      .value("AS", c_token_t::CT_AS)
      .value("IN", c_token_t::CT_IN)
      .value("BRACED", c_token_t::CT_BRACED)
      .value("THIS", c_token_t::CT_THIS)
      .value("BASE", c_token_t::CT_BASE)
      .value("DEFAULT", c_token_t::CT_DEFAULT)
      .value("GETSET", c_token_t::CT_GETSET)
      .value("GETSET_EMPTY", c_token_t::CT_GETSET_EMPTY)
      .value("CONCAT", c_token_t::CT_CONCAT)
      .value("CS_SQ_STMT", c_token_t::CT_CS_SQ_STMT)
      .value("CS_SQ_COLON", c_token_t::CT_CS_SQ_COLON)
      .value("CS_PROPERTY", c_token_t::CT_CS_PROPERTY)
      .value("SQL_EXEC", c_token_t::CT_SQL_EXEC)
      .value("SQL_BEGIN", c_token_t::CT_SQL_BEGIN)
      .value("SQL_END", c_token_t::CT_SQL_END)
      .value("SQL_WORD", c_token_t::CT_SQL_WORD)
      .value("SQL_ASSIGN", c_token_t::CT_SQL_ASSIGN)
      .value("CONSTRUCT", c_token_t::CT_CONSTRUCT)
      .value("LAMBDA", c_token_t::CT_LAMBDA)
      .value("ASSERT", c_token_t::CT_ASSERT)
      .value("ANNOTATION", c_token_t::CT_ANNOTATION)
      .value("FOR_COLON", c_token_t::CT_FOR_COLON)
      .value("DOUBLE_BRACE", c_token_t::CT_DOUBLE_BRACE)
      .value("CNG_HASINC", c_token_t::CT_CNG_HASINC)
      .value("CNG_HASINCN", c_token_t::CT_CNG_HASINCN)
      .value("Q_EMIT", c_token_t::CT_Q_EMIT)
      .value("Q_FOREACH", c_token_t::CT_Q_FOREACH)
      .value("Q_FOREVER", c_token_t::CT_Q_FOREVER)
      .value("Q_GADGET", c_token_t::CT_Q_GADGET)
      .value("Q_OBJECT", c_token_t::CT_Q_OBJECT)
      .value("MODE", c_token_t::CT_MODE)
      .value("DI", c_token_t::CT_DI)
      .value("HI", c_token_t::CT_HI)
      .value("QI", c_token_t::CT_QI)
      .value("SI", c_token_t::CT_SI)
      .value("NOTHROW", c_token_t::CT_NOTHROW)
      .value("WORD_", c_token_t::CT_WORD_);

   enum_<lang_flag_e>("Language")
      .value("C", lang_flag_e::LANG_C)
      .value("CPP", lang_flag_e::LANG_CPP)
      .value("D", lang_flag_e::LANG_D)
      .value("CS", lang_flag_e::LANG_CS)
      .value("JAVA", lang_flag_e::LANG_JAVA)
      .value("OC", lang_flag_e::LANG_OC)
      .value("VALA", lang_flag_e::LANG_VALA)
      .value("PAWN", lang_flag_e::LANG_PAWN)
      .value("ECMA", lang_flag_e::LANG_ECMA);

   // endregion enum bindings

   register_vector<std::string>("strings");

   class_<GenericOption>("GenericOption")
      .function("type", &GenericOption::type)
      .function("description", select_overload<std::string(const GenericOption &)>(
                   [](const GenericOption &o)
   {
      return((o.description() != nullptr) ? string(o.description()) : "");
   }))
      .function("name", select_overload<std::string(const GenericOption &)>(
                   [](const GenericOption &o)
   {
      return((o.name() != nullptr) ? string(o.name()) : "");
   }))
      .function("possible_values", select_overload<std::vector<std::string>(const GenericOption &)>(
                   [](const GenericOption &o)
   {
      std::vector<std::string> strings;

      auto ptr = o.possibleValues();

      for (auto c = *ptr; c; c = *++ptr)
      {
         strings.push_back(std::string{ c });
      }

      return(strings);
   }))
      .function("default", &GenericOption::defaultStr)
      .function("min", &GenericOption::minStr)
      .function("max", &GenericOption::maxStr)
      .function("is_default", &GenericOption::isDefault)
      .function("reset", &GenericOption::reset)
      .function("set", select_overload<bool(GenericOption &o, const std::string &s)>(
                   [](GenericOption &o, const std::string &s)
   {
      return(o.read(s.c_str()));
   }))
      .function("value", &GenericOption::str);

   register_vector<GenericOption *>("options");

   class_<Option<iarf_e>, base<GenericOption> >("OptionIARF")
      .function("value", &Option<iarf_e>::operator());

   class_<Option<line_end_e>, base<GenericOption> >("OptionLineEnd")
      .function("value", &Option<line_end_e>::operator());

   class_<Option<token_pos_e>, base<GenericOption> >("OptionTokenPos")
      .function("value", &Option<token_pos_e>::operator());

   class_<Option<unsigned>, base<GenericOption> >("OptionUnsigned")
      .function("value", &Option<unsigned>::operator());

   class_<Option<signed>, base<GenericOption> >("OptionSigned")
      .function("value", &Option<signed>::operator());

   class_<Option<std::string>, base<GenericOption> >("OptionString")
      .function("value", &Option<std::string>::operator());

   class_<OptionGroup>("OptionGroup")
      .property("description", select_overload<std::string(const OptionGroup &)>(
                   [](const OptionGroup &g)
   {
      return(std::string(g.description));
   }))
      .property("options", &OptionGroup::options);

   register_vector<OptionGroup *>("groups");

   emscripten::function("get_options", &get_options);
   emscripten::function("get_groups", &get_groups);

   emscripten::function("_initialize", &_initialize);
   emscripten::function("destruct", &destruct);

   emscripten::function("get_version", &get_version);

   emscripten::function("add_keyword", &_add_keyword);
   emscripten::function("clear_keywords", &clear_keywords);

   emscripten::function("reset_options", &reset_options);
   emscripten::function("option_reset_value", &reset_option);
   emscripten::function("option_set_value", &set_option);
   emscripten::function("option_get_value", &get_option);

   emscripten::function("_load_config", &_loadConfig);
   emscripten::function("show_config", select_overload<string(bool, bool)>(&show_config));
   emscripten::function("show_config", select_overload<string(bool)>(&show_config));
   emscripten::function("show_config", select_overload<string()>(&show_config));

   emscripten::function("log_type_enable", &log_set_sev);
   emscripten::function("log_type_show_name", &show_log_type);
   emscripten::function("quiet", &set_quiet);

   emscripten::function("_uncrustify", select_overload<intptr_t(intptr_t, lang_flag_e, bool, bool)>(&_uncrustify));
   emscripten::function("_uncrustify", select_overload<intptr_t(intptr_t, lang_flag_e, bool)>(&_uncrustify));
   emscripten::function("_uncrustify", select_overload<intptr_t(intptr_t, lang_flag_e)>(&_uncrustify));

   emscripten::function("_debug", select_overload<intptr_t(intptr_t, lang_flag_e, bool)>(&_debug));
   emscripten::function("_debug", select_overload<intptr_t(intptr_t, lang_flag_e)>(&_debug));
};

#endif
