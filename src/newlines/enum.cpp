/**
 * @file enum.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/enum.h"

#include "log_rules.h"
#include "newlines/add.h"
#include "newlines/double_newline.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


// enum {
// enum class angle_state_e : unsigned int {
// enum-key attr(optional) identifier(optional) enum-base(optional) { enumerator-list(optional) }
// enum-key attr(optional) nested-name-specifier(optional) identifier enum-base(optional) ; TODO
// enum-key         - one of enum, enum class or enum struct  TODO
// identifier       - the name of the enumeration that's being declared
// enum-base(C++11) - colon (:), followed by a type-specifier-seq
// enumerator-list  - comma-separated list of enumerator definitions
void newlines_enum(Chunk *start)
{
   LOG_FUNC_ENTRY();

   log_rule_B("nl_define_macro");

   if (  start->TestFlags(PCF_IN_PREPROC)
      && !options::nl_define_macro())
   {
      return;
   }
   // look for 'enum class'
   Chunk *pcClass = start->GetNextNcNnl();

   if (pcClass->Is(CT_ENUM_CLASS))
   {
      log_rule_B("nl_enum_class");
      newline_iarf_pair(start, pcClass, options::nl_enum_class());
      // look for 'identifier'/ 'type'
      Chunk *pcType = pcClass->GetNextNcNnl();

      if (pcType->Is(CT_TYPE))
      {
         log_rule_B("nl_enum_class_identifier");
         newline_iarf_pair(pcClass, pcType, options::nl_enum_class_identifier());
         // look for ':'
         Chunk *pcColon = pcType->GetNextNcNnl();

         if (pcColon->Is(CT_ENUM_COLON))                       // Issue #4040
         {
            log_rule_B("nl_enum_identifier_colon");
            newline_iarf_pair(pcType, pcColon, options::nl_enum_identifier_colon());
            // look for 'type' i.e. unsigned
            Chunk *pcType1 = pcColon->GetNextNcNnl();

            if (pcType1->Is(CT_TYPE))
            {
               log_rule_B("nl_enum_colon_type");
               newline_iarf_pair(pcColon, pcType1, options::nl_enum_colon_type());
               // look for 'type' i.e. int
               Chunk *pcType2 = pcType1->GetNextNcNnl();

               if (pcType2->Is(CT_TYPE))
               {
                  log_rule_B("nl_enum_colon_type");
                  newline_iarf_pair(pcType1, pcType2, options::nl_enum_colon_type());
               }
            }
         }
      }
   }
   /*
    * step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon or '=', which are not expected.
    */
   size_t level = start->GetLevel();
   Chunk  *pc   = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= level)
   {
      if (  pc->GetLevel() == level
         && (  pc->Is(CT_BRACE_OPEN)
            || pc->IsSemicolon()
            || pc->Is(CT_ASSIGN)))
      {
         break;
      }
      start = pc;
      pc    = pc->GetNextNcNnl();
   }

   // If we hit a brace open, then we need to toy with the newlines
   if (pc->Is(CT_BRACE_OPEN))
   {
      // Skip over embedded C comments
      Chunk *next = pc->GetNext();

      while (next->Is(CT_COMMENT))
      {
         next = next->GetNext();
      }
      iarf_e nl_opt;

      if (!next->IsCommentOrNewline())
      {
         nl_opt = IARF_IGNORE;
      }
      else
      {
         log_rule_B("nl_enum_brace");
         nl_opt = options::nl_enum_brace();
      }
      newline_iarf_pair(start, pc, nl_opt);
   }
} // newlines_enum


//! If requested, make sure each entry in an enum is on its own line
void newlines_enum_entries(Chunk *open_brace, iarf_e av)
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = open_brace->GetNextNc();
        pc->IsNotNullChunk() && pc->GetLevel() > open_brace->GetLevel();
        pc = pc->GetNextNc())
   {
      if (  (pc->GetLevel() != (open_brace->GetLevel() + 1))
         || pc->IsNot(CT_COMMA)
         || (  pc->Is(CT_COMMA)
            && (  pc->GetNext()->GetType() == CT_COMMENT_CPP
               || pc->GetNext()->GetType() == CT_COMMENT
               || pc->GetNext()->GetType() == CT_COMMENT_MULTI)))
      {
         continue;
      }
      newline_iarf(pc, av);
   }

   newline_iarf(open_brace, av);
} // newlines_enum_entries
