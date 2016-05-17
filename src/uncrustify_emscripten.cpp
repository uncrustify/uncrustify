/*
 * uncrustify_emscripten.cpp
 *
 *  Created on: May 8, 2016
 *      Author: Daniel Chumak
 */

#ifdef EMSCRIPTEN

#include "prototypes.h"
#include "options.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include <emscripten/bind.h>


#define STRINGIFY(s)    # s


using namespace std;
using namespace emscripten;

extern void process_option_line(char *configLine, const char *filename);


/**
 * Loads options from a file represented as a single char array.
 * Modifies: input char array, cpd.line_number
 *
 * @param configString char array that holds the whole config
 * @return EXIT_SUCCESS on success
 */
int load_option_fileChar(char *configString)
{
   const int textLen         = strlen(configString);
   char      *delimPos       = &configString[0];
   char      *stringEnd      = &configString[textLen - 1];
   char      *subStringStart = &configString[0];

   cpd.line_number = 0;

   while (true)
   {
      delimPos = std::find(delimPos, stringEnd, '\n');

      // replaces \n with \0 to get a string with multiple terminated
      // substrings inside
      *delimPos = '\0';

      process_option_line(subStringStart, "");

      if (delimPos == stringEnd)
      {
         break;
      }
      delimPos++;
      subStringStart = delimPos;
   }
   return(EXIT_SUCCESS);
}

// TODO: interface for args:
// -----------------------------------------------------------------------------
// --show-config
// --frag
// --show
// --type
// -l
// --log
// -q
// --version
//
//
// unsure about these:
// -----------------------------------------------------------------------------
// --check
// --decode
// --parsed
// --update-config
// --update-config-with-doc
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
//
// --config, -c ( use set_config( string _cfg ) )
// --file, -f ( use uncrustify( string _file ) )


/**
 * initializes the current libUncrustify instance,
 * used only for emscripten binding here and will be automatically called while
 * module initialization
 */
void initialize()
{
   register_options();
   set_option_defaults();
   log_init(stderr);

   LOG_FMT(LSYS, "Initialized libUncrustify\n");
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
int loadConfig(string _cfg)
{
   if (_cfg.empty())
   {
      LOG_FMT(LERR, "%s: input string is empty\n", __func__);
      return(EXIT_FAILURE);
   }

   unique_ptr<char[]> cfg(new char[_cfg.length() + 1]);
   std::strcpy(cfg.get(), _cfg.c_str());

   if (load_option_fileChar(cfg.get()) != EXIT_SUCCESS)
   {
      LOG_FMT(LERR, "unable to load the config\n");
      return(EXIT_FAILURE);
   }

   /* This relies on cpd.filename being the config file name */
   load_header_files();


   LOG_FMT(LSYS, "finished loading config\n");
   return(EXIT_SUCCESS);
}


/**
 * format string
 *
 * @param file: string that is going to be formated
 * @return formated string
 */
string uncrustify(string file)
{
   if (file.empty())
   {
      LOG_FMT(LERR, "%s: file string is empty\n", __func__);
      return("");
   }

   // Problem: uncrustify originally is not a lib and uses global vars such as
   // cpd.error_count for the whole program execution
   // to know if errors occurred during the formating step we reset this var here
   cpd.error_count = 0;

   if (cpd.lang_flags == 0)   // 0 == undefined
   {
      LOG_FMT(LWARN, "language of input file not defined, C++ will be assumed\n");
      cpd.lang_flags = LANG_CPP;
   }

   file_mem fm;
   fm.raw.clear();
   fm.data.clear();
   fm.enc = ENC_ASCII;
   fm.raw = vector<UINT8>(file.begin(), file.end());

   if (!decode_unicode(fm.raw, fm.data, fm.enc, fm.bom))
   {
      LOG_FMT(LERR, "Failed to read code\n");
      return("");
   }

   cpd.filename = "stdin";

   /* Done reading from stdin */
   LOG_FMT(LSYS, "Parsing: %d bytes (%d chars) from stdin as language %s\n",
           (int)fm.raw.size(), (int)fm.data.size(),
           language_name_from_flags(cpd.lang_flags));

   FILE   *stream;
   char   *buf;
   size_t len;

   // TODO (upstream): uncrustify uses FILE instead of streams for its outputs
   // to redirect FILE writes into a char* open_memstream is used
   // windows lacks open_memstream, only UNIX/BSD is supported
   // apparently emscripten has its own implementation, if that is not working
   // see: stackoverflow.com/questions/10305095#answer-10341073
   stream = open_memstream(&buf, &len);
   if (stream == NULL)
   {
      LOG_FMT(LERR, "Failed to open_memstream\n");
      fflush(stream);
      fclose(stream);
      free(buf);
      return("");
   }

   uncrustify_file(fm, stream, NULL);

   fflush(stream);
   fclose(stream);

   string out(buf);
   free(buf);

   if (cpd.error_count != 0)
   {
      LOG_FMT(LWARN, "%d errors occurred during formating\n", cpd.error_count);
   }
   return(out);
} // uncrustify


EMSCRIPTEN_BINDINGS(MainModule)
{
   emscripten::function(STRINGIFY(initialize), &initialize);
   emscripten::function(STRINGIFY(destruct), &destruct);

   emscripten::function(STRINGIFY(loadConfig), &loadConfig);

   emscripten::function(STRINGIFY(uncrustify), &uncrustify);
}

#endif
