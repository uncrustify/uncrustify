/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel
 *          October 2015- 2023
 * @license GPL v2+
 */

#include "unc_tools.h"

#include "args.h"
#include "output.h"


/*
 * the test suite Coveralls: https://coveralls.io
 * will complains because these functions are only
 * used at development time.
 * Don't worry about unused lines for the functions:
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
 *   prot_the_line(__func__, __LINE__, pc->GetOrigLine(), 0);
 *   prot_the_line(__func__, __LINE__, 0, 0);
 *   prot_the_line(__func__, __LINE__, 6, 5);
 *   prot_the_source(__LINE__);
 *   log_pcf_flags(LSYS, pc->GetFlags());
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
      if (pc->GetOrigLine() == actual_line)
      {
         tokenCounter++;

         if (  partNumber == 0
            || partNumber == tokenCounter)
         {
            LOG_FMT(LGUY, " orig line is %d, (%zu) ", actual_line, tokenCounter);

            if (pc->Is(CT_VBRACE_OPEN))
            {
               LOG_FMT(LGUY, "<VBRACE_OPEN>, ");
            }
            else if (pc->Is(CT_NEWLINE))
            {
               LOG_FMT(LGUY, "<NL>(new line count is %zu), ", pc->GetNlCount());
            }
            else if (pc->Is(CT_VBRACE_CLOSE))
            {
               LOG_FMT(LGUY, "<CT_VBRACE_CLOSE>, ");
            }
            else if (pc->Is(CT_VBRACE_OPEN))
            {
               LOG_FMT(LGUY, "<CT_VBRACE_OPEN>, ");
            }
            else if (pc->Is(CT_SPACE))
            {
               LOG_FMT(LGUY, "<CT_SPACE>, ");
            }
            else if (pc->Is(CT_IGNORED))
            {
               LOG_FMT(LGUY, "<IGNORED> ");
            }
            else
            {
               LOG_FMT(LGUY, "Text() '%s', ", pc->Text());
            }
            LOG_FMT(LGUY, " column is %zu, pp level is %zu, type is %s, parent type is %s, orig col is %zu,",
                    pc->GetColumn(), pc->GetPpLevel(), get_token_name(pc->GetType()),
                    get_token_name(pc->GetParentType()), pc->GetOrigCol());

            if (pc->Is(CT_IGNORED))
            {
               LOG_FMT(LGUY, "\n");
            }
            else
            {
               LOG_FMT(LGUY, " pc->GetFlags(): ");
               log_pcf_flags(LGUY, pc->GetFlags());
            }

            if (pc->Is(CT_COND_COLON))
            {
               Chunk *pa = pc->GetParent();
               LOG_FMT(LGUY, "<> pa-type is %s, orig_line is %zu\n", get_token_name(pa->GetType()), pa->GetOrigLine());
            }

            if (pc->GetTrackingData() != nullptr)
            {
               LOG_FMT(LGUY, " Tracking info are: \n");
               LOG_FMT(LGUY, "  number of track(s) %zu\n", pc->GetTrackingData()->size());

               for (size_t track = 0; track < pc->GetTrackingData()->size(); track++)
               {
                  const TrackList   *A       = pc->GetTrackingData();
                  const TrackNumber B        = A->at(track);
                  size_t            Bfirst   = B.first;
                  char              *Bsecond = B.second;

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

      LOG_FMT(LGUY, " orig line is %zu,%zu, pp level is %zu, ", lineNumber, tokenCounter, pc->GetPpLevel());

      if (pc->Is(CT_VBRACE_OPEN))
      {
         LOG_FMT(LGUY, "<VBRACE_OPEN>, ");
      }
      else if (pc->Is(CT_NEWLINE))
      {
         LOG_FMT(LGUY, "<NL>(new line count is %zu), ", pc->GetNlCount());
         tokenCounter = 0;
         lineNumber   = lineNumber + pc->GetNlCount();
      }
      else if (pc->Is(CT_VBRACE_CLOSE))
      {
         LOG_FMT(LGUY, "<CT_VBRACE_CLOSE>, ");
      }
      else if (pc->Is(CT_VBRACE_OPEN))
      {
         LOG_FMT(LGUY, "<CT_VBRACE_OPEN>, ");
      }
      else if (pc->Is(CT_SPACE))
      {
         LOG_FMT(LGUY, "<CT_SPACE>, ");
      }
      else if (pc->Is(CT_IGNORED))
      {
         LOG_FMT(LGUY, "<IGNORED> ");
      }
      else
      {
         LOG_FMT(LGUY, "Text() '%s', ", pc->Text());
      }
      LOG_FMT(LGUY, " column is %zu, type is %s\n",
              pc->GetColumn(), get_token_name(pc->GetType()));
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
         if (  pc->Is(CT_SQUARE_CLOSE)
            || pc->Is(CT_TSQUARE))
         {
            LOG_FMT(LGUY, "\n");
            LOG_FMT(LGUY, "1:(%d),", theLine);
            LOG_FMT(LGUY, "%s, orig col=%zu, orig col end=%zu\n", pc->Text(), pc->GetOrigCol(), pc->GetOrigColEnd());
         }
      }

      break;

   case 2:
      LOG_FMT(LGUY, "2:(%d)\n", theLine);

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (pc->GetOrigLine() == 7)
         {
            if (pc->Is(CT_NEWLINE))
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->GetOrigLine(), pc->GetOrigCol());
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->GetOrigLine(), pc->Text(), get_token_name(pc->GetType()), pc->GetOrigCol(), pc->GetColumn());
            }
         }
      }

      break;

   case 3:
      LOG_FMT(LGUY, "3:(%d)\n", theLine);

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (pc->Is(CT_NEWLINE))
         {
            LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->GetOrigLine(), pc->GetOrigCol());
         }
         else
         {
            LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->GetOrigLine(), pc->Text(), get_token_name(pc->GetType()), pc->GetOrigCol(), pc->GetColumn());
         }
      }

      break;

   case 4:
      LOG_FMT(LGUY, "4:(%d)\n", theLine);

      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         if (pc->GetOrigLine() == 6)
         {
            if (pc->Is(CT_NEWLINE))
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->GetOrigLine(), pc->GetOrigCol());
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->GetOrigLine(), pc->Text(), get_token_name(pc->GetType()), pc->GetOrigCol(), pc->GetColumn());
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
         fprintf(D_file, "  type %s\n", get_token_name(pc->GetType()));
         fprintf(D_file, "  orig line %zu\n", pc->GetOrigLine());
         fprintf(D_file, "  orig col %zu\n", pc->GetOrigCol());
         fprintf(D_file, "  orig col end %zu\n", pc->GetOrigColEnd());

         if (pc->GetOrigPrevSp() != 0)
         {
            fprintf(D_file, "  orig prev sp %zu\n", pc->GetOrigPrevSp());
         }

         if (pc->GetColumn() != 0)
         {
            fprintf(D_file, "  column %zu\n", pc->GetColumn());
         }

         if (pc->GetColumnIndent() != 0)
         {
            fprintf(D_file, "  column indent %zu\n", pc->GetColumnIndent());
         }

         if (pc->GetNlCount() != 0)
         {
            fprintf(D_file, "  nl_count %zu\n", pc->GetNlCount());
         }

         if (pc->GetLevel() != 0)
         {
            fprintf(D_file, "  level %zu\n", pc->GetLevel());
         }

         if (pc->GetBraceLevel() != 0)
         {
            fprintf(D_file, "  brace level %zu\n", pc->GetBraceLevel());
         }

         if (pc->GetPpLevel() != 0)
         {
            fprintf(D_file, "  pp level %zu\n", pc->GetPpLevel());
         }

         if (pc->GetAfterTab() != 0)
         {
            fprintf(D_file, "  after tab %d\n", pc->GetAfterTab());
         }

         if (pc->IsNot(CT_NEWLINE))
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
               chunk.CopyAndAddBefore(Chunk::NullChunkPtr);
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
               chunk.SetType(tokenName);
            }
            else if (strcasecmp(parts[0], "orig line") == 0)
            {
               chunk.SetOrigLine(strtol(parts[1], nullptr, 0));
            }
            else if (strcasecmp(parts[0], "orig col") == 0)
            {
               chunk.SetOrigCol(strtol(parts[1], nullptr, 0));
            }
            else if (strcasecmp(parts[0], "orig col end") == 0)
            {
               chunk.SetOrigColEnd(strtol(parts[1], nullptr, 0));
            }
            else if (strcasecmp(parts[0], "orig prev sp") == 0)
            {
               chunk.SetOrigPrevSp(strtol(parts[1], nullptr, 0));
            }
            else if (strcasecmp(parts[0], "column") == 0)
            {
               chunk.SetColumn(strtol(parts[1], nullptr, 0));
            }
            else if (strcasecmp(parts[0], "nl_count") == 0)
            {
               chunk.SetNlCount(strtol(parts[1], nullptr, 0));
            }
            else if (strcasecmp(parts[0], "text") == 0)
            {
               if (chunk.GetType() != CT_NEWLINE)
               {
                  chunk.Str() = parts[1];
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
      chunk.CopyAndAddBefore(Chunk::NullChunkPtr);
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


void dump_keyword_for_lang(size_t language_count, chunk_tag_t *keyword_for_lang)
{
   LOG_FMT(LDYNKW, "%s:                      tag            type        flags\n", __func__);

   for (size_t ii = 0; ii < language_count; ii++)
   {
      std::bitset<16> bit_a;
      bit_a = keyword_for_lang[ii].lang_flags;
      std::string     g_a;
      g_a = bit_a.to_string();
      LOG_FMT(LDYNKW, "%s: %3zu: %18s, %14s, %12ld, %16s, %s\n",
              __func__, ii,
              keyword_for_lang[ii].tag, get_token_name(keyword_for_lang[ii].type),
              keyword_for_lang[ii].lang_flags,
              g_a.data(),
              language_name_from_flags(keyword_for_lang[ii].lang_flags));
   }
}
