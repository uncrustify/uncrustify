/**
 * @file class_colon_pos.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/class_colon_pos.h"

#include "align/stack.h"
#include "chunk.h"
#include "keywords.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/add.h"
#include "newlines/force.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newlines_class_colon_pos(E_Token tok)
{
   LOG_FUNC_ENTRY();

   token_pos_e tpc;
   token_pos_e pcc;
   iarf_e      anc;
   iarf_e      ncia;

   if (tok == CT_CLASS_COLON)
   {
      tpc = options::pos_class_colon();
      log_rule_B("pos_class_colon");
      anc = options::nl_class_colon();
      log_rule_B("nl_class_colon");
      ncia = options::nl_class_init_args();
      log_rule_B("nl_class_init_args");
      pcc = options::pos_class_comma();
      log_rule_B("pos_class_comma");
   }
   else // tok == CT_CONSTR_COLON
   {
      tpc = options::pos_constr_colon();
      log_rule_B("pos_constr_colon");
      anc = options::nl_constr_colon();
      log_rule_B("nl_constr_colon");
      ncia = options::nl_constr_init_args();
      log_rule_B("nl_constr_init_args");
      pcc = options::pos_constr_comma();
      log_rule_B("pos_constr_comma");
   }
   size_t acv_span = options::align_constr_value_span();

   log_rule_B("align_constr_value_span");
   bool       with_acv = (acv_span > 0) && language_is_set(lang_flag_e::LANG_CPP);
   AlignStack constructorValue;    // ABC_Member(abc_value)

   if (with_acv)
   {
      int    acv_thresh = options::align_constr_value_thresh();
      log_rule_B("align_constr_value_thresh");
      size_t acv_gap = options::align_constr_value_gap();
      log_rule_B("align_constr_value_gap");
      constructorValue.Start(acv_span, acv_thresh);
      constructorValue.m_gap         = acv_gap;
      constructorValue.m_right_align = !options::align_on_tabstop();
      log_rule_B("align_on_tabstop");
   }
   Chunk *ccolon = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  ccolon->IsNullChunk()
         && pc->IsNot(tok))
      {
         continue;
      }
      Chunk *prev;
      Chunk *next;

      if (pc->Is(tok))
      {
         LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
         ccolon = pc;
         prev   = pc->GetPrevNc();
         next   = pc->GetNextNc();

         if (pc->Is(CT_CONSTR_COLON))
         {
            LOG_FMT(LBLANKD, "%s(%d): pc orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            Chunk *paren_vor_value = pc->GetNextType(CT_FPAREN_OPEN, pc->GetLevel());

            if (  with_acv
               && paren_vor_value->IsNotNullChunk())
            {
               LOG_FMT(LBLANKD, "%s(%d): paren_vor_value orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                       __func__, __LINE__, paren_vor_value->GetOrigLine(), paren_vor_value->GetOrigCol(),
                       paren_vor_value->Text(), get_token_name(paren_vor_value->GetType()));
               constructorValue.NewLines(paren_vor_value->GetNlCount());
               constructorValue.Add(paren_vor_value);
            }
         }

         if (  !prev->IsNewline()
            && !next->IsNewline()
            && (anc & IARF_ADD))                   // nl_class_colon, nl_constr_colon: 1

         {
            newline_add_after(pc);
            prev = pc->GetPrevNc();
            next = pc->GetNextNc();
         }

         if (anc == IARF_REMOVE)                   // nl_class_colon, nl_constr_colon: 2
         {
            if (  prev->IsNewline()
               && prev->SafeToDeleteNl())
            {
               Chunk::Delete(prev);
               MARK_CHANGE();
               prev = pc->GetPrevNc();
            }

            if (  next->IsNewline()
               && next->SafeToDeleteNl())
            {
               Chunk::Delete(next);
               MARK_CHANGE();
               next = pc->GetNextNc();
            }
         }

         if (tpc & TP_TRAIL)                       // pos_class_colon, pos_constr_colon: 4
         {
            if (  prev->IsNewline()
               && prev->GetNlCount() == 1
               && prev->SafeToDeleteNl())
            {
               pc->Swap(prev);
            }
         }
         else if (tpc & TP_LEAD)                   // pos_class_colon, pos_constr_colon: 3
         {
            if (  next->IsNewline()
               && next->GetNlCount() == 1
               && next->SafeToDeleteNl())
            {
               pc->Swap(next);
            }
         }
      }
      else
      {
         // (pc->GetType() != tok)
         if (  pc->Is(CT_BRACE_OPEN)
            || pc->Is(CT_SEMICOLON))
         {
            ccolon = Chunk::NullChunkPtr;

            if (with_acv)
            {
               constructorValue.End();
            }
            continue;
         }

         if (  pc->Is(CT_COMMA)
            && pc->GetLevel() == ccolon->GetLevel())
         {
            LOG_FMT(LBLANKD, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
            Chunk *paren_vor_value = pc->GetNextType(CT_FPAREN_OPEN, pc->GetLevel());

            if (  with_acv
               && paren_vor_value->IsNotNullChunk())
            {
               LOG_FMT(LBLANKD, "%s(%d): paren_vor_value orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                       __func__, __LINE__, paren_vor_value->GetOrigLine(), paren_vor_value->GetOrigCol(),
                       paren_vor_value->Text(), get_token_name(paren_vor_value->GetType()));
               constructorValue.NewLines(paren_vor_value->GetNlCount());
               constructorValue.Add(paren_vor_value);
            }

            if (ncia & IARF_ADD)                   // nl_class_init_args, nl_constr_init_args:
            {
               if (pcc & TP_TRAIL)                 // pos_class_comma, pos_constr_comma
               {
                  if (ncia == IARF_FORCE)          // nl_class_init_args, nl_constr_init_args: 5
                  {
                     Chunk *after = pc->GetNext();   // Issue #2759

                     if (after->IsNot(CT_COMMENT_CPP))
                     {
                        newline_force_after(pc);
                     }
                  }
                  else
                  {
                     // (ncia == IARF_ADD)         // nl_class_init_args, nl_constr_init_args: 8
                     newline_add_after(pc);
                  }
                  prev = pc->GetPrevNc();

                  if (  prev->IsNewline()
                     && prev->SafeToDeleteNl())
                  {
                     Chunk::Delete(prev);
                     MARK_CHANGE();
                  }
               }
               else if (pcc & TP_LEAD)             // pos_class_comma, pos_constr_comma
               {
                  if (ncia == IARF_FORCE)          // nl_class_init_args, nl_constr_init_args: 7
                  {
                     newline_force_before(pc);
                  }
                  else
                  {
                     // (ncia == IARF_ADD)         // nl_class_init_args, nl_constr_init_args: 9
                     newline_add_before(pc);
                  }
                  next = pc->GetNextNc();

                  if (  next->IsNewline()
                     && next->SafeToDeleteNl())
                  {
                     Chunk::Delete(next);
                     MARK_CHANGE();
                  }
               }
            }
            else if (ncia == IARF_REMOVE)          // nl_class_init_args, nl_constr_init_args: 6
            {
               next = pc->GetNext();

               if (  next->IsNewline()
                  && next->SafeToDeleteNl())
               {
                  // comma is after
                  Chunk::Delete(next);
                  MARK_CHANGE();
               }
               else
               {
                  prev = pc->GetPrev();

                  if (  prev->IsNewline()
                     && prev->SafeToDeleteNl())
                  {
                     // comma is before
                     Chunk::Delete(prev);
                     MARK_CHANGE();
                  }
               }
            }
         }
      }
   }
} // newlines_class_colon_pos
