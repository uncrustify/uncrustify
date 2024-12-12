/**
 * @file chunk_pos.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/chunk_pos.h"

#include "chunk.h"
#include "log_rules.h"
#include "newlines/add.h"
#include "newlines/remove_next_newlines.h"
#include "uncrustify.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


void newlines_chunk_pos(E_Token chunk_type, uncrustify::token_pos_e mode)
{
   LOG_FUNC_ENTRY();

   if (  chunk_type == CT_BOOL
      && options::code_width() > 0
      && options::nl_bool_expression_multiline()
      && (  options::pos_bool() == TP_LEAD
         || options::pos_bool() == TP_TRAIL))
   {
      // here treat as join and let do_code_width do the heavy lifting
      mode = TP_JOIN;
   }
   LOG_FMT(LNEWLINE, "%s(%d): mode is %s\n",
           __func__, __LINE__, to_string(mode));

   if (  !(mode & (TP_JOIN | TP_LEAD | TP_TRAIL))
      && chunk_type != CT_COMMA)
   {
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      char copy[1000];
      LOG_FMT(LNEWLINE, "%s(%d): pc orig line is %zu, orig col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));
      // produces much more log output. Use it only debugging purpose
      //log_pcf_flags(LNEWLINE, pc->GetFlags());

      if (pc->Is(chunk_type))
      {
         token_pos_e mode_local;

         if (chunk_type == CT_COMMA)
         {
            LOG_FMT(LNEWLINE, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            // produces much more log output. Use it only debugging purpose
            //log_pcf_flags(LNEWLINE, pc->GetFlags());

            if (pc->TestFlags(PCF_IN_CONST_ARGS)) // Issue #2250
            {
               continue;
            }

            /*
             * for chunk_type == CT_COMMA
             * we get 'mode' from options::pos_comma()
             * BUT we must take care of options::pos_class_comma()
             * TODO and options::pos_constr_comma()
             */
            if (pc->TestFlags(PCF_IN_CLASS_BASE))
            {
               // change mode
               log_rule_B("pos_class_comma");
               mode_local = options::pos_class_comma();
            }
            else if (pc->TestFlags(PCF_IN_ENUM))
            {
               log_rule_B("pos_enum_comma");
               mode_local = options::pos_enum_comma();
            }
            else
            {
               mode_local = mode;
            }
            LOG_FMT(LNEWLINE, "%s(%d): mode_local is %s\n",
                    __func__, __LINE__, to_string(mode_local));
         }
         else
         {
            mode_local = mode;
         }
         Chunk *prev = pc->GetPrevNc();
         Chunk *next = pc->GetNextNc();

         LOG_FMT(LNEWLINE, "%s(%d): mode_local is %s\n",
                 __func__, __LINE__, to_string(mode_local));

         LOG_FMT(LNEWLINE, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());
         LOG_FMT(LNEWLINE, "%s(%d): next orig line is %zu, orig col is %zu, Text() is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text());
         size_t nl_flag = ((prev->IsNewline() ? 1 : 0) |
                           (next->IsNewline() ? 2 : 0));
         LOG_FMT(LNEWLINE, "%s(%d): nl_flag is %zu\n",
                 __func__, __LINE__, nl_flag);

         if (mode_local & TP_JOIN)
         {
            if (nl_flag & 1)
            {
               // remove newline if not preceded by a comment
               Chunk *prev2 = prev->GetPrev();

               if (  prev2->IsNotNullChunk()
                  && !(prev2->IsComment()))
               {
                  remove_next_newlines(prev2);
               }
            }

            if (nl_flag & 2)
            {
               // remove newline if not followed by a comment or by '{'
               Chunk *next2 = next->GetNext();

               if (  next2->IsNotNullChunk()
                  && !next2->IsComment()
                  && !next2->Is(CT_BRACE_OPEN))
               {
                  remove_next_newlines(pc);
               }
            }
            continue;
         }

         if (  (  nl_flag == 0
               && !(mode_local & (TP_FORCE | TP_BREAK)))
            || (  nl_flag == 3
               && !(mode_local & TP_FORCE)))
         {
            // No newlines and not adding any or both and not forcing
            continue;
         }

         if (  (  (mode_local & TP_LEAD)
               && nl_flag == 1)
            || (  (mode_local & TP_TRAIL)
               && nl_flag == 2))
         {
            // Already a newline before (lead) or after (trail)
            continue;
         }

         // If there were no newlines, we need to add one
         if (nl_flag == 0)
         {
            if (mode_local & TP_LEAD)
            {
               newline_add_before(pc);
            }
            else
            {
               newline_add_after(pc);
            }
            continue;
         }

         // If there were both newlines, we need to remove one
         if (nl_flag == 3)
         {
            if (mode_local & TP_LEAD)
            {
               remove_next_newlines(pc);
            }
            else
            {
               remove_next_newlines(pc->GetPrevNcNnlNi());   // Issue #2279
            }
            continue;
         }

         // we need to move the newline
         if (mode_local & TP_LEAD)
         {
            Chunk *next2 = next->GetNext();

            if (  next2->Is(CT_PREPROC)
               || (  chunk_type == CT_ASSIGN
                  && next2->Is(CT_BRACE_OPEN)))
            {
               continue;
            }

            if (next->GetNlCount() == 1)
            {
               if (  prev->IsNotNullChunk()
                  && !prev->TestFlags(PCF_IN_PREPROC))
               {
                  // move the CT_BOOL to after the newline
                  pc->MoveAfter(next);
               }
            }
         }
         else
         {
            LOG_FMT(LNEWLINE, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s', new line count is %zu\n",
                    __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text(), prev->GetNlCount());

            if (prev->GetNlCount() == 1)
            {
               // Back up to the next non-comment item
               prev = prev->GetPrevNc();
               LOG_FMT(LNEWLINE, "%s(%d): prev orig line is %zu, orig col is %zu, Text() is '%s'\n",
                       __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());

               if (  prev->IsNotNullChunk()
                  && !prev->IsNewline()
                  && !prev->TestFlags(PCF_IN_PREPROC)
                  && !prev->TestFlags(PCF_IN_OC_MSG))
               {
                  pc->MoveAfter(prev);
               }
            }
         }
      }
   }
} // newlines_chunk_pos
