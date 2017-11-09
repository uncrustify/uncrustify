/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "unc_tools.h"
#include "uncrustify.h"
#include "args.h"
#include "output.h"


static size_t counter = 0;
static size_t tokenCounter;


// protocol of the line
// examples:
//   prot_the_line(__LINE__, pc->orig_line);
//   prot_the_line(__LINE__, 6);
//   prot_the_source(__LINE__);
// log_pcf_flags(LSYS, pc->flags);
void prot_the_line(int theLine, unsigned int actual_line)
{
   counter++;
   tokenCounter = 0;
   LOG_FMT(LGUY, "Prot_the_line:(%d)(%zu)\n", theLine, counter);
   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = pc->next)
   {
      if (pc->orig_line == actual_line)
      {
         tokenCounter++;
         LOG_FMT(LGUY, " orig_line is %d, ", actual_line);
         if (pc->type == CT_VBRACE_OPEN)
         {
            LOG_FMT(LGUY, "<VBRACE_OPEN>, ");
         }
         else if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LGUY, "<NL>(%zu), ", pc->nl_count);
         }
         else if (pc->type == CT_VBRACE_CLOSE)
         {
            LOG_FMT(LGUY, "<CT_VBRACE_CLOSE>, ");
         }
         else if (pc->type == CT_VBRACE_OPEN)
         {
            LOG_FMT(LGUY, "<CT_VBRACE_OPEN>, ");
         }
         else if (pc->type == CT_SPACE)
         {
            LOG_FMT(LGUY, "<CT_SPACE>, ");
         }
         else
         {
            LOG_FMT(LGUY, "(%zu) text() %s, type is %s, parent_type is %s, orig_col is %zu, column is %zu, ",
                    tokenCounter, pc->text(), get_token_name(pc->type), get_token_name(pc->parent_type), pc->orig_col, pc->column);
         }
         LOG_FMT(LGUY, "pc->flags:");
         log_pcf_flags(LGUY, pc->flags);
      }
   }
   LOG_FMT(LGUY, "\n");
} // prot_the_line


void prot_the_source(int theLine)
{
   counter++;
   LOG_FMT(LGUY, "Prot_the_source:(%d)(%zu)\n", theLine, counter);
   output_text(stderr);
}


// TODO: examine_Data seems not to be used, is it still required?


// examples:
//   examine_Data(__func__, __LINE__, n);
void examine_Data(const char *func_name, int theLine, int what)
{
   LOG_FMT(LGUY, "\n%s:", func_name);

   chunk_t *pc;
   switch (what)
   {
   case 1:
      for (pc = chunk_get_head(); pc != nullptr; pc = pc->next)
      {
         if (pc->type == CT_SQUARE_CLOSE || pc->type == CT_TSQUARE)
         {
            LOG_FMT(LGUY, "\n");
            LOG_FMT(LGUY, "1:(%d),", theLine);
            LOG_FMT(LGUY, "%s, orig_col=%zu, orig_col_end=%zu\n", pc->text(), pc->orig_col, pc->orig_col_end);
         }
      }
      break;

   case 2:
      LOG_FMT(LGUY, "2:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != nullptr; pc = pc->next)
      {
         if (pc->orig_line == 7)
         {
            if (pc->type == CT_NEWLINE)
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
            }
         }
      }
      break;

   case 3:
      LOG_FMT(LGUY, "3:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != nullptr; pc = pc->next)
      {
         if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
         }
         else
         {
            LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
         }
      }
      break;

   case 4:
      LOG_FMT(LGUY, "4:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != nullptr; pc = pc->next)
      {
         if (pc->orig_line == 6)
         {
            if (pc->type == CT_NEWLINE)
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
            }
         }
      }
      break;

   default:
      break;
   } // switch
}    // examine_Data


void dump_out(unsigned int type)
{
   char dumpFileName[300];

   if (cpd.dumped_file == nullptr)
   {
      sprintf(dumpFileName, "%s.%u", cpd.filename.c_str(), type);
   }
   else
   {
      sprintf(dumpFileName, "%s.%u", cpd.dumped_file, type);
   }
   FILE *D_file = fopen(dumpFileName, "w");
   if (D_file != nullptr)
   {
      for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = pc->next)
      {
         fprintf(D_file, "[%p]\n", pc);
         fprintf(D_file, "  type %s\n", get_token_name(pc->type));
         fprintf(D_file, "  orig_line %zu\n", pc->orig_line);
         fprintf(D_file, "  orig_col %zu\n", pc->orig_col);
         fprintf(D_file, "  orig_col_end %zu\n", pc->orig_col_end);
         if (pc->orig_prev_sp != 0)
         {
            fprintf(D_file, "  orig_prev_sp %u\n", pc->orig_prev_sp);
         }
         if (pc->flags != 0)
         {
            fprintf(D_file, "  flags %" PRIu64 "\n", pc->flags);
         }
         if (pc->column != 0)
         {
            fprintf(D_file, "  column %zu\n", pc->column);
         }
         if (pc->column_indent != 0)
         {
            fprintf(D_file, "  column_indent %zu\n", pc->column_indent);
         }
         if (pc->nl_count != 0)
         {
            fprintf(D_file, "  nl_count %zu\n", pc->nl_count);
         }
         if (pc->level != 0)
         {
            fprintf(D_file, "  level %zu\n", pc->level);
         }
         if (pc->brace_level != 0)
         {
            fprintf(D_file, "  brace_level %zu\n", pc->brace_level);
         }
         if (pc->pp_level != 0)
         {
            fprintf(D_file, "  pp_level %zu\n", pc->pp_level);
         }
         if (pc->after_tab != 0)
         {
            fprintf(D_file, "  after_tab %d\n", pc->after_tab);
         }
         if (pc->type != CT_NEWLINE)
         {
            fprintf(D_file, "  text %s\n", pc->text());
         }
      }
      fclose(D_file);
   }
} // dump_out


void dump_in(unsigned int type)
{
   char    buffer[256];
   bool    aNewChunkIsFound = false;
   chunk_t chunk;
   char    dumpFileName[300];

   if (cpd.dumped_file == nullptr)
   {
      sprintf(dumpFileName, "%s.%u", cpd.filename.c_str(), type);
   }
   else
   {
      sprintf(dumpFileName, "%s.%u", cpd.dumped_file, type);
   }
   FILE *D_file = fopen(dumpFileName, "r");

   if (D_file != nullptr)
   {
      unsigned int lineNumber = 0;
      while (fgets(buffer, sizeof(buffer), D_file) != nullptr)
      {
         ++lineNumber;
         if (aNewChunkIsFound)
         {
            // look for the next chunk
            char first = buffer[0];
            if (first == '[')
            {
               aNewChunkIsFound = false;
               // add the chunk in the list
               chunk_add_before(&chunk, nullptr);
               chunk.reset();
               aNewChunkIsFound = true;
               continue;
            }
            // the line as the form
            // part value
            // Split the line
#define NUMBER_OF_PARTS    3
            char *parts[NUMBER_OF_PARTS];
            int partCount = Args::SplitLine(buffer, parts, NUMBER_OF_PARTS - 1);
            if (partCount != 2)
            {
               exit(EX_SOFTWARE);
            }

            if (strcasecmp(parts[0], "type") == 0)
            {
               c_token_t tokenName = find_token_name(parts[1]);
               set_chunk_type(&chunk, tokenName);
            }
            else if (strcasecmp(parts[0], "orig_line") == 0)
            {
               chunk.orig_line = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "orig_col") == 0)
            {
               chunk.orig_col = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "orig_col_end") == 0)
            {
               chunk.orig_col_end = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "orig_prev_sp") == 0)
            {
               chunk.orig_prev_sp = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "flags") == 0)
            {
               chunk.flags = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "column") == 0)
            {
               chunk.column = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "nl_count") == 0)
            {
               chunk.nl_count = strtol(parts[1], nullptr, 0);
            }
            else if (strcasecmp(parts[0], "text") == 0)
            {
               if (chunk.type != CT_NEWLINE)
               {
                  chunk.str = parts[1];
               }
            }
            else
            {
               fprintf(stderr, "on line=%d, for '%s'\n", lineNumber, parts[0]);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
         }
         else
         {
            // look for a new chunk
            char first = buffer[0];
            if (first == '[')
            {
               aNewChunkIsFound = true;
               chunk.reset();
            }
         }
      }
      // add the last chunk in the list
      chunk_add_before(&chunk, nullptr);
      fclose(D_file);
   }
   else
   {
      fprintf(stderr, "FATAL: file not found '%s'\n", dumpFileName);
      log_flush(true);
      exit(EX_SOFTWARE);
   }
} // dump_in
