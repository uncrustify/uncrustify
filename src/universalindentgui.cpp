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
#include "prototypes.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_version.h"

#include <vector>

#include <cstdio>


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
   fprintf(pfile, "%s=", key);
   for (auto c = option->possibleValues(); *c; ++c)
   {
      fprintf(pfile, "%s=%s%c", option->name(), *c, c[1] ? '|' : '\n');
   }
}


void print_universal_indent_cfg(FILE *pfile)
{
   const char *p_name;

   // Dump the header and the categories
   fprintf(pfile, "[header]\n");

   // Add all the categories
   const auto &groups = get_option_groups();
   char       ch      = '=';

   fprintf(pfile, "categories");
   for (auto *const g : groups)
   {
      fputc(ch, pfile);
      ch = '|';

      // Write description, stripping leading and trailing newlines
      for (auto dc = g->description + 1; *(dc + 1); ++dc)
      {
         fputc(*dc, pfile);
      }
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

#ifdef DEBUG
   size_t optionNumber = 0;
#endif // DEBUG
   // Now add each option
   for (size_t idx = 0; idx < groups.size(); ++idx)
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

         fprintf(pfile, "\n[%s]\n", optionNameReadable);
         fprintf(pfile, "Category=%zu\n", idx);
#ifdef DEBUG
         fprintf(pfile, "Description=\"<html>(%zu)", optionNumber);
         optionNumber++;
#else    // DEBUG
         fprintf(pfile, "Description=\"<html>");
#endif

         // Skip first character, which is always a newline
         const char *tmp = option->description() + 1;
         ch = 0;

         // Output the description which may contain forbidden chars, skipping
         // the last character which is always an extra newline
         while (*tmp != 0 && *(tmp + 1) != 0)
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
         if (option == &uncrustify::options::indent_with_tabs)
         {
            // Indenting with tabs selector becomes a multiple selector and not
            // only a number. Also it is by default enabled.
            fprintf(pfile, "Enabled=true\n");
            fprintf(pfile, "EditorType=multiple\n");
            fprintf(pfile, "Choices=\"%s=0|%s=1|%s=2\"\n",
                    option->name(), option->name(), option->name());
            fprintf(pfile, "ChoicesReadable=\"Spaces only|Indent with tabs, align with spaces|Indent and align with tabs\"\n");
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
               // [align_keep_tabs]
               // Category=3
               // Description=<html>Whether to keep non-indenting tabs</html>
               // Value=0
               // ValueDefault=0
               // EditorType=boolean
               // TrueFalse="align_keep_tabs=true|align_keep_tabs=false"
               fprintf(pfile, "EditorType=boolean\n");
               print_option_choices(pfile, option, "TrueFalse");
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_IARF:
               fprintf(pfile, "EditorType=multiple\n");
               print_option_choices(pfile, option);
               fprintf(pfile, "ChoicesReadable=\"Ignore %s|Add %s|Remove %s|Force %s\"\n",
                       optionNameReadable, optionNameReadable, optionNameReadable, optionNameReadable);
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               // [nl_after_switch]
               // Category=4
               // Description=<html>Add or remove newline after 'switch'</html>
               // Value=3
               // ValueDefault=-1
               // Enabled=true
               // EditorType=multiple
               // Choices="nl_after_switch=ignore|nl_after_switch=add|nl_after_switch=remove|nl_after_switch=force"
               break;

            case uncrustify::OT_NUM:
               // [align_assign_span]
               // CallName="align_assign_span="
               // Category=3
               // Description="<html>The span for aligning on '=' in assignments (0=don't align)</html>"
               // EditorType=numeric
               // Enabled=false
               // MaxVal=300
               // MinVal=0
               // Value=0
               // ValueDefault=0
               fprintf(pfile, "EditorType=numeric\n");
               fprintf(pfile, "CallName=\"%s=\"\n", option->name());
               fprintf(pfile, "MinVal=%s\n", option->minStr().c_str());
               fprintf(pfile, "MaxVal=%s\n", option->maxStr().c_str());
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_UNUM:
               // [input_tab_size]
               // CallName="input_tab_size="
               // Category=3
               // Description="<html>The original size of tabs in the input. Default=8</html>"
               // EditorType=numeric
               // Enabled=false
               // MaxVal=32
               // MinVal=1
               // ValueDefault=8
               fprintf(pfile, "EditorType=numeric\n");
               fprintf(pfile, "CallName=\"%s=\"\n", option->name());
               fprintf(pfile, "MinVal=%s\n", option->minStr().c_str());
               fprintf(pfile, "MaxVal=%s\n", option->maxStr().c_str());
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_LINEEND:
               // [newlines]
               // Category=0
               // Description=<html>The type of line endings</html>
               // Value=0
               // ValueDefault=-1
               // Enabled=true
               // EditorType=multiple
               // Choices="newlines=auto|newlines=lf|newlines=crlf|newlines=cr"
               fprintf(pfile, "EditorType=multiple\n");
               print_option_choices(pfile, option);
               fprintf(pfile, "ChoicesReadable=\"Newlines Unix|Newlines Win|Newlines Mac|Newlines Auto\"\n");
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_TOKENPOS:
               // [pos_bool]
               // Category=5
               // Description=<html>The position of boolean operators in wrapped expressions</html>
               // Enabled=false
               // Value=0
               // ValueDefault=-1
               // EditorType=multiple
               // Choices="pos_bool=ignore|pos_bool=lead|pos_bool=trail"
               fprintf(pfile, "EditorType=multiple\n");
               print_option_choices(pfile, option);
               fprintf(pfile, "ChoicesReadable=\"Ignore %s|Lead %s|Lead Break %s|Lead Force %s|Trail %s|Trail Break %s|Trail Force %s\"\n",
                       optionNameReadable, optionNameReadable, optionNameReadable,
                       optionNameReadable, optionNameReadable, optionNameReadable,
                       optionNameReadable);
               fprintf(pfile, "ValueDefault=%s\n", option->str().c_str());
               break;

            case uncrustify::OT_STRING:
            {
               fprintf(pfile, "CallName=%s=\n", option->name());
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
         delete[] optionNameReadable;
      }
   }
} // print_universal_indent_cfg
