/**
 * @file language_names.cpp
 *
 * @author  Guy Maurel
 * extract from uncrustify.cpp
 * @license GPL v2+
 */
#include "language_names.h"

#include "keywords.h"

static lang_name_t language_names[] =
{
   { "C",        e_LANG_C                           },  // 0x0001
   { "CPP",      e_LANG_CPP                         },  // 0x0002
   { "D",        e_LANG_D                           },  // 0x0004
   { "CS",       e_LANG_CS                          },  // 0x0008
   { "JAVA",     e_LANG_JAVA                        },  // 0x0010
   { "OC",       e_LANG_OC                          },  // 0x0020
   { "VALA",     e_LANG_VALA                        },  // 0x0040
   { "PAWN",     e_LANG_PAWN                        },  // 0x0080
   { "ECMA",     e_LANG_ECMA                        },  // 0x0100
   { "OC+",      e_LANG_OC | e_LANG_CPP             },  // 0x0020 + 0x0002
   { "CS+",      e_LANG_CS | e_LANG_CPP             },  // 0x0008 + 0x0002
   { "C-Header", e_LANG_C | e_LANG_CPP | e_FLAG_HDR },  // 0x0001 + 0x0002 + 0x2000 = 0x2022
};


//! known filename extensions linked to the corresponding programming language
struct lang_ext_t language_exts[] =
{
   { ".c",    "C"        },
   { ".c++",  "CPP"      },
   { ".cc",   "CPP"      },
   { ".cp",   "CPP"      },
   { ".cpp",  "CPP"      },
   { ".cs",   "CS"       },
   { ".cxx",  "CPP"      },
   { ".d",    "D"        },
   { ".di",   "D"        },
   { ".es",   "ECMA"     },
   { ".h",    "C-Header" },
   { ".h++",  "CPP"      },
   { ".hh",   "CPP"      },
   { ".hp",   "CPP"      },
   { ".hpp",  "CPP"      },
   { ".hxx",  "CPP"      },
   { ".inc",  "PAWN"     },
   { ".inl",  "CPP"      },
   { ".java", "JAVA"     },
   { ".js",   "ECMA"     },
   { ".m",    "OC"       },
   { ".mm",   "OC+"      },
   { ".p",    "PAWN"     },
   { ".pawn", "PAWN"     },
   { ".sma",  "PAWN"     },
   { ".sqc",  "C"        },  // embedded SQL
   { ".sql",  "SQL"      },
   { ".vala", "VALA"     },
   { ".vapi", "VALA"     },
};


size_t language_flags_from_name(const char *name)
{
   for (const auto &language : language_names)
   {
      if (strcasecmp(name, language.name) == 0)
      {
         return(language.lang);
      }
   }

   return(0);
}


const char *language_name_from_flags(size_t lang)
{
   // Check for an exact match first
   for (auto &language_name : language_names)
   {
      if (language_name.lang == lang)
      {
         return(language_name.name);
      }
   }

   static char lang_liste[120];
   lang_liste[0] = '\0';

   // Check for the next set language bit
   for (auto &language_name : language_names)
   {
      if (strcmp(language_name.name, "OC+") == 0)
      {
         break;
      }

      if ((language_name.lang & lang) != 0)
      {
         if (lang_liste[0] == '\0')
         {
            strcpy(lang_liste, language_name.name);
         }
         else
         {
            int ll = strlen(lang_liste);
            strcpy(&lang_liste[ll], ", ");
            strcpy(&lang_liste[ll + 2], language_name.name);
         }
      }
   }

   return(lang_liste);
} // language_name_from_flags


bool ends_with(const char *filename, const char *tag, bool case_sensitive = true)
{
   int len1 = strlen(filename);
   int len2 = strlen(tag);

   return(  len2 <= len1
         && (  (  case_sensitive
               && (strcmp(&filename[len1 - len2], tag) == 0))
            || (  !case_sensitive
               && (strcasecmp(&filename[len1 - len2], tag) == 0))));
} // ends_with


const char *get_file_extension(int &idx)
{
   const char *val = nullptr;

   if (idx < static_cast<int> ARRAY_SIZE(language_exts))
   {
      val = language_exts[idx].ext;
   }
   idx++;
   return(val);
} // get_file_extension


////typedef std::map<std::string, std::string> extension_map_t;
//static extension_map_t g_ext_map;


const char *extension_add(const char *ext_text, const char *lang_text)
{
   size_t lang_flags = language_flags_from_name(lang_text);

   if (lang_flags)
   {
      const char *lang_name = language_name_from_flags(lang_flags);
      g_ext_map[std::string(ext_text)] = lang_name;
      return(lang_name);
   }
   return(nullptr);
}


void print_extensions(FILE *pfile)
{
   for (auto &language : language_names)
   {
      bool did_one = false;

      for (auto &extension_val : g_ext_map)
      {
         if (strcmp(extension_val.second.c_str(), language.name) == 0)
         {
            if (!did_one)
            {
               fprintf(pfile, "file_ext %s", extension_val.second.c_str());
               did_one = true;
            }
            fprintf(pfile, " %s", extension_val.first.c_str());
         }
      }

      if (did_one)
      {
         fprintf(pfile, "\n");
      }
   }
}


// TODO: better use enum lang_t for source file language
size_t language_flags_from_filename(const char *filename)
{
   // check custom extensions first
   for (const auto &extension_val : g_ext_map)
   {
      if (ends_with(filename, extension_val.first.c_str()))
      {
         return(language_flags_from_name(extension_val.second.c_str()));
      }
   }

   for (auto &language : language_exts)
   {
      if (ends_with(filename, language.ext))
      {
         return(language_flags_from_name(language.name));
      }
   }

   // check again without case sensitivity
   for (auto &extension_val : g_ext_map)
   {
      if (ends_with(filename, extension_val.first.c_str(), false))
      {
         return(language_flags_from_name(extension_val.second.c_str()));
      }
   }

   for (auto &language : language_exts)
   {
      if (ends_with(filename, language.ext, false))
      {
         return(language_flags_from_name(language.name));
      }
   }

   return(e_LANG_C);
} // language_flags_from_filename
