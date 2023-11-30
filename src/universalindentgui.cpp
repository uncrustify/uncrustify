/**
 * @file universalindentgui.cpp
 * Exports the config file for UniversalIndentGUI
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "universalindentgui.h"

#include "error_types.h"
#include "log_rules.h"
#include "prototypes.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_version.h"

#include <cstdio>
#include <vector>


constexpr static auto LCURRENT = LOTHER;

using namespace std;


std::vector<uncrustify::OptionGroup *> get_option_groups()
{
   std::vector<uncrustify::OptionGroup *> groups;
   size_t                                 i = 0;

   while (auto *const g = uncrustify::get_option_group(i))
   {
      groups.push_back(g);
      ++i;
   }
   return(groups);
}


void print_option_choices(FILE *pfile, uncrustify::GenericOption *option,
                          char const *key = "Choices")
{
   // Normal string
   fprintf(pfile, "%s=", key);

   for (auto c = option->possibleValues(); *c; ++c)
   {
      fprintf(pfile, "%s=%s%c", option->name(), *c, c[1] ? '|' : '\n');
   }

   // Regex string
   fprintf(pfile, "%sRegex=", key);

   for (auto c = option->possibleValues(); *c; ++c)
   {
      fprintf(pfile, "%s\\s*=\\s*%s%c", option->name(), *c, c[1] ? '|' : '\n');
   }
}


void print_universal_indent_cfg(FILE *pfile)
{
   const char *p_name;
   char       ch      = '=';
   const auto &groups = get_option_groups();
   size_t     idx;

#if defined (DEBUG) && !defined (WIN32)
   vector<size_t> allGroups;
   allGroups.reserve(16);
   // first run to get the first option number of each group/category
   size_t optionNumber         = 0;
   bool   firstOptionNumberSet = false;

   for (idx = 0; idx < groups.size(); ++idx)
   {
      const auto *p_grp = groups[idx];

      for (auto *const option : p_grp->options)
      {
         UNUSED(option);

         if (!firstOptionNumberSet)
         {
            allGroups[idx]       = optionNumber;
            firstOptionNumberSet = true;
         }
         optionNumber++;
      } // for (auto *const option : p_grp->options)

      firstOptionNumberSet = false;
   } // end of first run

//#else
//   UNUSED(allGroups);
#endif // DEBUG

   // second run
   // Dump the header and the categories
   fprintf(pfile, "[header]\n");

   // Add all the categories
   //const auto &groups = get_option_groups();
   ch = '=';

   fprintf(pfile, "categories");
   idx = 0;
#if defined (DEBUG) && !defined (WIN32)
   optionNumber = 0;
#endif // DEBUG

   for (auto *const g : groups)
   {
      fputc(ch, pfile);
      ch = '|';

#if defined (DEBUG) && !defined (WIN32)
      fprintf(pfile, "(%zu)", allGroups[idx]);
#endif // DEBUG

      // Write description, stripping leading and trailing newlines
      for (auto dc = g->description + 1; *(dc + 1); ++dc)
      {
         fputc(*dc, pfile);
      }

      idx++;
   }

   fprintf(pfile, "\n");

   fprintf(pfile,
           "cfgFileParameterEnding=cr\n"
           "configFilename=uncrustify.cfg\n");


   // Add all the recognized file extensions
   ch = '=';
   int fileIdx = 0;
   fprintf(pfile, "fileTypes");

   while ((p_name = get_file_extension(fileIdx)) != nullptr)
   {
      fprintf(pfile, "%c*%s", ch, p_name);
      ch = '|';
   }
   fprintf(pfile, "\n");

   // Add the rest of the constant file header
   fprintf(pfile,
           "indenterFileName=uncrustify\n"
           "indenterName=Uncrustify (C, C++, C#, ObjectiveC, D, Java, Pawn, VALA)\n"
           "inputFileName=indentinput\n"
           "inputFileParameter=\"-f \"\n"
           "manual=http://uncrustify.sourceforge.net/config.txt\n"
           "outputFileName=indentoutput\n"
           "outputFileParameter=\"-o \"\n"
           "stringparaminquotes=false\n"
           "parameterOrder=ipo\n"
           "showHelpParameter=-h\n"
           "stringparaminquotes=false\n"
           "useCfgFileParameter=\"-c \"\n");

   fprintf(pfile, "version=%s\n", UNCRUSTIFY_VERSION);

   ch = '=';

   // Now add each option
   for (idx = 0; idx < groups.size(); ++idx)
   {
      const auto *p_grp = groups[idx];

      for (auto *const option : p_grp->options)
      {
         /*
          * Create a better readable name from the options name
          * by replacing '_' by a space and use some upper case characters.
          */
         char *optionNameReadable = new char[strlen(option->name()) + 1];
         strcpy(optionNameReadable, option->name());

         bool was_space = true;

         for (char *character = optionNameReadable; *character != 0; character++)
         {
            if (*character == '_')
            {
               *character = ' ';
               was_space  = true;
            }
            else if (was_space)
            {
               *character = unc_toupper(*character);
               was_space  = false;
            }
         }

#if defined (DEBUG) && !defined (WIN32)
         fprintf(pfile, "\n[(%zu)%s]\n", optionNumber, optionNameReadable);
#else // DEBUG
         fprintf(pfile, "\n[%s]\n", optionNameReadable);
#endif // DEBUG
         fprintf(pfile, "Category=%zu\n", idx);
#if defined (DEBUG) && !defined (WIN32)
         fprintf(pfile, "Description=\"<html>(%zu)", optionNumber);
#else // DEBUG
         fprintf(pfile, "Description=\"<html>");
#endif // DEBUG

         // Skip first character, which is always a newline
         const char *tmp = option->description() + 1;
         ch = 0;

         // Output the description which may contain forbidden chars, skipping
         // the last character which is always an extra newline
         while (  *tmp != 0
               && *(tmp + 1) != 0)
         {
            switch (*tmp)
            {
            case '<':
               fputs("&lt;", pfile);
               break;

            case '>':
               fputs("&gt;", pfile);
               break;

            case '&':
               fputs("&amp;", pfile);
               break;

            case '\n':
               fputs("<br/>", pfile);
               break;

            default:
               fputc(*tmp, pfile);
            }
            tmp++;
         }
         const auto ds = option->defaultStr();

         if (!ds.empty())
         {
            fprintf(pfile, "<br/><br/>Default: %s", ds.c_str());
         }
         fprintf(pfile, "</html>\"\n");

         // Handle some options independent of their type and most by their type.
         log_rule_B("indent_with_tabs");

         if (option == &uncrustify::options::indent_with_tabs)
         {
            // Indenting with tabs selector becomes a multiple selector and not
            // only a number. Also it is by default enabled.
            fprintf(pfile, "Enabled=true\n");
            fprintf(pfile, "EditorType=multiple\n");
            fprintf(pfile, "Choices=\"%s=0|%s=1|%s=2\"\n",
                    option->name(), option->name(), option->name());
            fprintf(pfile, "ChoicesRegex=\"%s\\s*=\\s*0|%s\\s*=\\s*1|%s\\s*=\\s*2\"\n",
                    option->name(), option->name(), option->name());
#if defined (DEBUG) && !defined (WIN32)
            fprintf(pfile, "ChoicesReadable=\"(%zu)Spaces only|(%zu)Indent with tabs, align with spaces|(%zu)Indent and align with tabs\"\n",
                    optionNumber, optionNumber, optionNumber);
#else // DEBUG
            fprintf(pfile, "ChoicesReadable=\"Spaces only|Indent with tabs, align with spaces|Indent and align with tabs\"\n");
#endif // DEBUG
            fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
         }
         else
         {
            // All not specially handled options are created only dependent by
            // their type.
            fprintf(pfile, "Enabled=false\n");

            switch (option->type())
            {
            case uncrustify::OT_BOOL:
               fprintf(pfile, "EditorType=boolean\n");
               print_option_choices(pfile, option, "TrueFalse");
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_IARF:
               fprintf(pfile, "EditorType=multiple\n");
               print_option_choices(pfile, option);
#if defined (DEBUG) && !defined (WIN32)
               fprintf(pfile, "ChoicesReadable=\"(%zu)Ignore %s|(%zu)Add %s|(%zu)Remove %s|(%zu)Force %s\"\n",
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable);
#else // DEBUG
               //                                0         1      2         3
               fprintf(pfile, "ChoicesReadable=\"Ignore %s|Add %s|Remove %s|Force %s\"\n",
                       optionNameReadable, optionNameReadable, optionNameReadable, optionNameReadable);
#endif // DEBUG
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_NUM:
               fprintf(pfile, "EditorType=numeric\n");
               fprintf(pfile, "CallName=\"%s=\"\n", option->name());
               fprintf(pfile, "CallNameRegex=\"%s\\s*=\\s*\"\n", option->name());
               fprintf(pfile, "MinVal=%s\n", option->minStr().c_str());
               fprintf(pfile, "MaxVal=%s\n", option->maxStr().c_str());
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_UNUM:
               fprintf(pfile, "EditorType=numeric\n");
               fprintf(pfile, "CallName=\"%s=\"\n", option->name());
               fprintf(pfile, "CallNameRegex=\"%s\\s*=\\s*\"\n", option->name());
               fprintf(pfile, "MinVal=%s\n", option->minStr().c_str());
               fprintf(pfile, "MaxVal=%s\n", option->maxStr().c_str());
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_LINEEND:
               fprintf(pfile, "EditorType=multiple\n");
               print_option_choices(pfile, option);
#if defined (DEBUG) && !defined (WIN32)
               fprintf(pfile, "ChoicesReadable=\"(%zu)Newlines Unix|(%zu)Newlines Win|(%zu)Newlines Mac|(%zu)Newlines Auto\"\n",
                       optionNumber, optionNumber, optionNumber, optionNumber);
#else // DEBUG
               fprintf(pfile, "ChoicesReadable=\"Newlines Unix|Newlines Win|Newlines Mac|Newlines Auto\"\n");
#endif // DEBUG
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_TOKENPOS:
               fprintf(pfile, "EditorType=multiple\n");
               // Issue #2300-a
               print_option_choices(pfile, option);
#if defined (DEBUG) && !defined (WIN32)
               fprintf(pfile, "ChoicesReadable=\"(%zu)Ignore %s|(%zu)Break %s|(%zu)Force %s|(%zu)Lead %s|(%zu)Trail %s|",
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable);
#else // DEBUG
               //                                0         1        2        4       8
               fprintf(pfile, "ChoicesReadable=\"Ignore %s|Break %s|Force %s|Lead %s|Trail %s|",
                       optionNameReadable, optionNameReadable, optionNameReadable,
                       optionNameReadable, optionNameReadable);
#endif // DEBUG
               //                                16      5             6             9              10
#if defined (DEBUG) && !defined (WIN32)
               fprintf(pfile, "(%zu)Join %s|(%zu)Lead Break %s|(%zu)Lead Force %s|(%zu)Trail Break %s|(%zu)Trail Force %s\"\n",
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable,
                       optionNumber, optionNameReadable);
#else // DEBUG
               fprintf(pfile, "Join %s|Lead Break %s|Lead Force %s|Trail Break %s|Trail Force %s\"\n",
                       optionNameReadable, optionNameReadable, optionNameReadable,
                       optionNameReadable, optionNameReadable);
#endif // DEBUG
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_STRING:
            {
               fprintf(pfile, "CallName=%s=\n", option->name());
               fprintf(pfile, "CallNameRegex=%s\\s*=\\s*\n", option->name());
               fprintf(pfile, "EditorType=string\n");
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;
            }

            default:
               fprintf(stderr, "FATAL: Illegal option type %d for '%s'\n",
                       static_cast<int>(option->type()), option->name());
               log_flush(true);
               exit(EX_SOFTWARE);
               break;
            } // switch
         }
#if defined (DEBUG) && !defined (WIN32)
         optionNumber++;
#endif // DEBUG
         delete[] optionNameReadable;
      } // for (auto *const option : p_grp->options)
   }
} // print_universal_indent_cfg
