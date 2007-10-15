/**
 * @file universalindentgui.cpp
 * Exports the config file for UniversalIndentGUI
 *
 * $Id$
 */
#include "prototypes.h"
#include <stdio.h>



void print_universal_indent_cfg(FILE *pfile)
{
   const group_map_value *p_grp;
   const char            *p_name;

   /* Dump the header and the categories */
   fprintf(pfile, "[%%20header]\n");

   /* Add all the categories */
   char ch = '=';
   int  idx;

   fprintf(pfile, "categories");
   for (idx = 0; idx < UG_group_count; idx++)
   {
      p_grp = get_group_name(idx);
      if (p_grp != NULL)
      {
         fprintf(pfile, "%c%s", ch, p_grp->short_desc);
         ch = '|';
      }
   }
   fprintf(pfile, "\n");

   fprintf(pfile,
           "cfgFileParameterEnding=cr\n"
           "configFilename=uncrustify.cfg\n");


   /* Add all the recognized file extensions */
   ch  = '=';
   idx = 0;
   fprintf(pfile, "fileTypes");
   while ((p_name = get_file_extension(idx)) != NULL)
   {
      fprintf(pfile, "%c*%s", ch, p_name);
      ch = '|';
   }
   fprintf(pfile, "\n");

   /* Add the rest of the constant file header */
   fprintf(pfile,
           "indenterFileName=uncrustify\n"
           "indenterName=Uncrustify\n"
           "inputFileName=indentinput\n"
           "inputFileParameter=\"-f \"\n"
           "outputFileName=indentoutput\n"
           "outputFileParameter=\"-o \"\n"
           "stringparaminquotes=false\n"
           "parameterOrder=ipo\n"
           "useCfgFileParameter=\"-c \"\n");

   /* Now add each option */
   for (idx = 0; idx < UG_group_count; idx++)
   {
      p_grp = get_group_name(idx);
      if (p_grp == NULL)
      {
         continue;
      }

      for (option_list_cit it = p_grp->options.begin(); it != p_grp->options.end(); it++)
      {
         const option_map_value *option = get_option_name(*it);

         fprintf(pfile, "\n[%s]\n", option->name);
         fprintf(pfile, "Category=%d\n", idx);
         fprintf(pfile, "Description=\"<html>");

         const char *tmp = option->short_desc;
         ch = 0;

         /* Output the decription which may contain forbidden chars */
         while (*tmp != 0)
         {
            switch (*tmp)
            {
            case '<':
               fprintf(pfile, "&lt;");
               break;

            case '>':
               fprintf(pfile, "&gt;");
               break;

            case '&':
               fprintf(pfile, "&amp;");
               break;

            case '\n':
               fprintf(pfile, "<BR>");
               break;

            default:
               fputc(*tmp, pfile);
            }
            tmp++;
         }

         fprintf(pfile, "</html>\"\n");
         fprintf(pfile, "Value=%d\n", cpd.settings[option->id].n);
         fprintf(pfile, "ValueDefault=%d\n", cpd.settings[option->id].n);
         fprintf(pfile, "Enabled=false\n");

         switch (option->type)
         {
         case AT_BOOL:
            // [align_keep_tabs]
            // Category=3
            // Description=<html>Whether to keep non-indenting tabs</html>
            // Value=0
            // ValueDefault=0
            // EditorType=boolean
            // TrueFalse="align_keep_tabs=true|align_keep_tabs=false"
            fprintf(pfile, "EditorType=boolean\n");
            fprintf(pfile, "TrueFalse=%s=true|%s=false\n", option->name, option->name);
            break;

         case AT_IARF:
            fprintf(pfile, "EditorType=multiple\n");
            fprintf(pfile, "Choices=%s=ignore|%s=add|%s=remove|%s=force\n",
                    option->name, option->name, option->name, option->name);
            // [nl_after_switch]
            // Category=4
            // Description=<html>Add or remove newline after 'switch'</html>
            // Value=3
            // ValueDefault=-1
            // Enabled=true
            // EditorType=multiple
            // Choices="nl_after_switch=ignore|nl_after_switch=add|nl_after_switch=remove|nl_after_switch=force"
            break;

         case AT_NUM:
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
            fprintf(pfile, "CallName=\"%s=\"\n", option->name);
            fprintf(pfile, "MinVal=%d\n", option->min_val);
            fprintf(pfile, "MaxVal=%d\n", option->max_val);
            break;

         case AT_LINE:
            // [newlines]
            // Category=0
            // Description=<html>The type of line endings</html>
            // Value=0
            // ValueDefault=-1
            // Enabled=true
            // EditorType=multiple
            // Choices="newlines=auto|newlines=lf|newlines=crlf|newlines=cr"
            fprintf(pfile, "EditorType=multiple\n");
            fprintf(pfile, "Choices=%s=lf|%s=crlf|%s=cr|%s=auto\n",
                    option->name, option->name, option->name, option->name);
            break;

         case AT_POS:
            // [pos_bool]
            // Category=5
            // Description=<html>The position of boolean operators in wrapped expressions</html>
            // Enabled=false
            // Value=0
            // ValueDefault=-1
            // EditorType=multiple
            // Choices="pos_bool=ignore|pos_bool=lead|pos_bool=trail"
            fprintf(pfile, "EditorType=multiple\n");
            fprintf(pfile, "Choices=%s=ignore|%s=lead|%s=trail\n",
                    option->name, option->name, option->name);
            break;

         case AT_STRING:
            // TODO: figure out what is needed for string options
            // fprintf(pfile, "EditorType=multiple\n");
            // fprintf(pfile, "Choices=%s=ignore|%s=lead|%s=trail\n",
            //         option->name, option->name, option->name);
            break;

         default:
            break;
         }
      }
   }
}
