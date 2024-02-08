/**
 * @file func_pre_blank_lines.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/func_pre_blank_lines.h"

#include "chunk.h"
#include "log_rules.h"
#include "newlines/do_it_newlines_func_pre_blank_lines.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


/**
 * Add one/two newline(s) before the chunk.
 * Adds before comments
 * Adds before destructor
 * Doesn't do anything if open brace before it
 * "code\n\ncomment\nif (...)" or "code\ncomment\nif (...)"
 */
void newlines_func_pre_blank_lines(Chunk *start, E_Token start_type)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_before_func_class_def");
   log_rule_B("nl_before_func_class_proto");
   log_rule_B("nl_before_func_body_def");
   log_rule_B("nl_before_func_body_proto");

   if (  start->IsNullChunk()
      || (  (  start_type != CT_FUNC_CLASS_DEF
            || options::nl_before_func_class_def() == 0)
         && (  start_type != CT_FUNC_CLASS_PROTO
            || options::nl_before_func_class_proto() == 0)
         && (  start_type != CT_FUNC_DEF
            || options::nl_before_func_body_def() == 0)
         && (  start_type != CT_FUNC_PROTO
            || options::nl_before_func_body_proto() == 0)))
   {
      return;
   }
   LOG_FMT(LNLFUNCT, "%s(%d):    set blank line(s): for <NL> at line %zu, column %zu, start_type is %s\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), get_token_name(start_type));
   LOG_FMT(LNLFUNCT, "%s(%d): BEGIN set blank line(s) for '%s' at line %zu\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine());
   /*
    * look backwards until we find:
    *   - open brace (don't add or remove)
    *   - two newlines in a row (don't add)
    *   - a destructor
    *   - something else (don't remove)
    */
   Chunk  *pc           = Chunk::NullChunkPtr;
   Chunk  *last_nl      = Chunk::NullChunkPtr;
   Chunk  *last_comment = Chunk::NullChunkPtr;
   size_t first_line    = start->GetOrigLine();

   for (pc = start->GetPrev(); pc->IsNotNullChunk(); pc = pc->GetPrev())
   {
      LOG_FMT(LNLFUNCT, "%s(%d): orig line is %zu, orig col is %zu, type is %s, Text() is '%s', new line count is %zu\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->Text(), pc->GetNlCount());

      if (pc->IsNewline())
      {
         last_nl = pc;
         LOG_FMT(LNLFUNCT, "%s(%d):    <Chunk::IsNewline> found at line %zu, column %zu, new line count is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());
         LOG_FMT(LNLFUNCT, "%s(%d):    last_nl set to %zu\n",
                 __func__, __LINE__, last_nl->GetOrigLine());
         bool break_now = false;

         if (pc->GetNlCount() > 1)
         {
            break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
            LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                    __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         }

         if (break_now)
         {
            break;
         }
         else
         {
            continue;
         }
      }
      else if (pc->IsComment())
      {
         LOG_FMT(LNLFUNCT, "%s(%d):    <Chunk::IsComment> found at line %zu, column %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

         if (  (  pc->GetOrigLine() < first_line
               && ((first_line - pc->GetOrigLine()
                    - (pc->Is(CT_COMMENT_MULTI) ? pc->GetNlCount() : 0))) < 2)
            || (  last_comment->IsNotNullChunk()
               && pc->Is(CT_COMMENT_CPP)          // combine only cpp comments
               && last_comment->Is(pc->GetType()) // don't mix comment types
               && last_comment->GetOrigLine() > pc->GetOrigLine()
               && (last_comment->GetOrigLine() - pc->GetOrigLine()) < 2))
         {
            last_comment = pc;
            continue;
         }
         bool break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
         LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                 __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         continue;
      }
      else if (  pc->Is(CT_DESTRUCTOR)
              || pc->Is(CT_TYPE)
              || pc->Is(CT_TEMPLATE)
              || pc->Is(CT_QUALIFIER)
              || pc->Is(CT_PTR_TYPE)
              || pc->Is(CT_BYREF)                  // Issue #2163
              || pc->Is(CT_DC_MEMBER)
              || pc->Is(CT_EXTERN)
              || (  pc->Is(CT_STRING)
                 && pc->GetParentType() == CT_EXTERN))
      {
         LOG_FMT(LNLFUNCT, "%s(%d): first_line set to %zu\n",
                 __func__, __LINE__, pc->GetOrigLine());
         first_line = pc->GetOrigLine();
         continue;
      }
      else if (  pc->Is(CT_ANGLE_CLOSE)
              && pc->GetParentType() == CT_TEMPLATE)
      {
         LOG_FMT(LNLFUNCT, "%s(%d):\n", __func__, __LINE__);
         // skip template stuff to add newlines before it
         pc = pc->GetOpeningParen();

         if (pc->IsNotNullChunk())
         {
            first_line = pc->GetOrigLine();
         }
         continue;
      }
      else
      {
         LOG_FMT(LNLFUNCT, "%s(%d): else ==================================\n",
                 __func__, __LINE__);
         bool break_now = do_it_newlines_func_pre_blank_lines(last_nl, start_type);
         LOG_FMT(LNLFUNCT, "%s(%d): break_now is %s\n",
                 __func__, __LINE__, break_now ? "TRUE" : "FALSE");
         break;
      }
   }
} // newlines_func_pre_blank_lines
