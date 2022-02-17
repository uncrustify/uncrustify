/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel
 *          October 2015- 2021
 * @license GPL v2+
 */

#include "unc_tools.h"

#include "args.h"
#include "output.h"


/*
 * the test suite Coveralls: https://coveralls.io
 * will complains because these functions are only
 * used at developement time.
 * Don't worry about unsed lines for the functions:
 *   prot_the_line
 *   prot_the_source
 *   examine_Data
 *   dump_out
 *   dump_in
 */

static size_t counter = 0;
static size_t tokenCounter;


/* protocol of the line
 * examples:
 *   prot_the_line(__func__, __LINE__, pc->orig_line, 0);
 *   prot_the_line(__func__, __LINE__, 0, 0);
 *   prot_the_line(__func__, __LINE__, 6, 5);
 *   prot_the_source(__LINE__);
 *   log_pcf_flags(LSYS, pc->flags);
 *
 * if actual_line is zero, use the option debug_line_number_to_protocol.
 * if the value is zero, don't make any protocol and return.
 *
 * if partNumber is zero, all the tokens of the line are shown,
 * if partNumber is NOT zero, only the token with this partNumber is shown.
 *
 *   prot_the_line_pc(pc_sub, __func__, __LINE__, 6, 5);
 * to get a protocol of a sub branch, which begins with pc_sub
 */
void prot_the_line(const char *func_name, int theLine, unsigned int actual_line, size_t partNumber)
{
   prot_the_line_pc(Chunk::GetHead(), func_name, theLine, actual_line, partNumber);
}


void prot_the_line_pc(Chunk *pc_sub, const char *func_name, int theLine, unsigned int actual_line, size_t partNumber)
{
   if (actual_line == 0)
   {
      // use the option debug_line_number_to_protocol.
      actual_line = options::debug_line_number_to_protocol();

      if (actual_line == 0)
      {
         // don't make any protocol.
         return;
      }
   }
   counter++;
   tokenCounter = 0;
   LOG_FMT(LGUY, "Prot_the_line:(%s:%d)(%zu)\n", func_name, theLine, counter);

   for (Chunk *pc = pc_sub; pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->orig_line == actual_line)
      {
         tokenCounter++;

         if (  partNumber == 0
            || partNumber == tokenCounter)
         {
            LOG_FMT(LGUY, " orig_line is %d, (%zu) ", actual_line, tokenCounter);

            if (chunk_is_token(pc, CT_VBRACE_OPEN))
            {
               LOG_FMT(LGUY, "<VBRACE_OPEN>, ");
            }
            else if (chunk_is_token(pc, CT_NEWLINE))
            {
               LOG_FMT(LGUY, "<NL>(nl_count is %zu), ", pc->nl_count);
            }
            else if (chunk_is_token(pc, CT_VBRACE_CLOSE))
            {
               LOG_FMT(LGUY, "<CT_VBRACE_CLOSE>, ");
            }
            else if (chunk_is_token(pc, CT_VBRACE_OPEN))
            {
               LOG_FMT(LGUY, "<CT_VBRACE_OPEN>, ");
            }
            else if (chunk_is_token(pc, CT_SPACE))
            {
               LOG_FMT(LGUY, "<CT_SPACE>, ");
            }
            else if (chunk_is_token(pc, CT_IGNORED))
            {
               LOG_FMT(LGUY, "<IGNORED> ");
            }
            else
            {
               LOG_FMT(LGUY, "Text() '%s', ", pc->Text());
            }
            LOG_FMT(LGUY, " column is %zu, pp_level is %zu, type is %s, parent_type is %s, orig_col is %zu,",
                    pc->column, pc->pp_level, get_token_name(pc->type),
                    get_token_name(get_chunk_parent_type(pc)), pc->orig_col);

            if (chunk_is_token(pc, CT_IGNORED))
            {
               LOG_FMT(LGUY, "\n");
            }
            else
            {
               LOG_FMT(LGUY, " pc->flags: ");
               log_pcf_flags(LGUY, pc->flags);
            }

            if (pc->tracking != nullptr)
            {
               LOG_FMT(LGUY, " Tracking info are: \n");
               LOG_FMT(LGUY, "  number of track(s) %zu\n", pc->tracking->size());

               for (size_t track = 0; track < pc->tracking->size(); track++)
               {
                  track_list *A       = pc->tracking;
                  Track_nr   B        = A->at(track);
                  size_t     Bfirst   = B.first;
                  char       *Bsecond = B.second;

                  LOG_FMT(LGUY, "  %zu, tracking number is %zu\n", track, Bfirst);
                  LOG_FMT(LGUY, "  %zu, rule            is %s\n", track, Bsecond);
               }
            }
         }
      }
   }

   LOG_FMT(LGUY, "\n");
} // prot_the_line_pc


void prot_all_lines(const char *func_name, int theLine)
{
   counter++;
   tokenCounter = 0;
   size_t lineNumber = 1;

   LOG_FMT(LGUY, "Prot_all_lines:(%s:%d)(%zu)\n", func_name, theLine, counter);

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      tokenCounter++;

      LOG_FMT(LGUY, " orig_line is %zu,%zu, pp_level is %zu, ", lineNumber, tokenCounter, pc->pp_level);

      if (chunk_is_token(pc, CT_VBRACE_OPEN))
      {
         LOG_FMT(LGUY, "<VBRACE_OPEN>, ");
      }
      else if (chunk_is_token(pc, CT_NEWLINE))
      {
         LOG_FMT(LGUY, "<NL>(nl_count is %zu), ", pc->nl_count);
         tokenCounter = 0;
         lineNumber   = lineNumber + pc->nl_count;
      }
      else if (chunk_is_token(pc, CT_VBRACE_CLOSE))
      {
         LOG_FMT(LGUY, "<CT_VBRACE_CLOSE>, ");
      }
      else if (chunk_is_token(pc, CT_VBRACE_OPEN))
      {
         LOG_FMT(LGUY, "<CT_VBRACE_OPEN>, ");
      }
      else if (chunk_is_token(pc, CT_SPACE))
      {
         LOG_FMT(LGUY, "<CT_SPACE>, ");
      }
      else if (chunk_is_token(pc, CT_IGNORED))
      {
         LOG_FMT(LGUY, "<IGNORED> ");
      }
      else
      {
         LOG_FMT(LGUY, "Text() '%s', ", pc->Text());
      }
      LOG_FMT(LGUY, " column is %zu, type is %s\n",
              pc->column, get_token_name(pc->type));
   }
} // prot_all_lines


void prot_the_source(int theLine)
{
   counter++;
   LOG_FMT(LGUY, "Prot_the_source:(%d)(%zu)\n", theLine, counter);
   output_text(stderr);
}


// examples:
//   examine_Data(__func__, __LINE__, n);
void examine_Data(const char *func_name, int theLine, int what)
{
   LOG_FMT(LGUY, "\n%s:", func_name);

   Chunk *pc;

   switch (what)
   {
   case 1:

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (  chunk_is_token(pc, CT_SQUARE_CLOSE)
            || chunk_is_token(pc, CT_TSQUARE))
         {
            LOG_FMT(LGUY, "\n");
            LOG_FMT(LGUY, "1:(%d),", theLine);
            LOG_FMT(LGUY, "%s, orig_col=%zu, orig_col_end=%zu\n", pc->Text(), pc->orig_col, pc->orig_col_end);
         }
      }

      break;

   case 2:
      LOG_FMT(LGUY, "2:(%d)\n", theLine);

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (pc->orig_line == 7)
         {
            if (chunk_is_token(pc, CT_NEWLINE))
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->Text(), get_token_name(pc->type), pc->orig_col, pc->column);
            }
         }
      }

      break;

   case 3:
      LOG_FMT(LGUY, "3:(%d)\n", theLine);

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (chunk_is_token(pc, CT_NEWLINE))
         {
            LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
         }
         else
         {
            LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->Text(), get_token_name(pc->type), pc->orig_col, pc->column);
         }
      }

      break;

   case 4:
      LOG_FMT(LGUY, "4:(%d)\n", theLine);

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (pc->orig_line == 6)
         {
            if (chunk_is_token(pc, CT_NEWLINE))
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->Text(), get_token_name(pc->type), pc->orig_col, pc->column);
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
      for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
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
            fprintf(D_file, "  Text %s\n", pc->Text());
         }
      }

      fclose(D_file);
   }
} // dump_out


void dump_in(unsigned int type)
{
   char  buffer[256];
   bool  aNewChunkIsFound = false;
   Chunk chunk;
   char  dumpFileName[300];

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
               chunk.Reset();
               aNewChunkIsFound = true;
               continue;
            }
            // the line as the form
            // part value
            // Split the line
            const int max_parts_count = 3;
            char      *parts[max_parts_count];
            int       parts_count = Args::SplitLine(buffer, parts, max_parts_count - 1);

            if (parts_count != 2)
            {
               exit(EX_SOFTWARE);
            }

            if (strcasecmp(parts[0], "type") == 0)
            {
               E_Token tokenName = find_token_name(parts[1]);
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
               chunk.Reset();
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


size_t number = 0;


size_t get_A_Number()
{
   number = number + 1;
   return(number);
}
