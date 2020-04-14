/**
 * @file combine_fix.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "combine_fix.h"

#include "chunk_list.h"
#include "combine_labels.h"
#include "combine_mark.h"
#include "combine_skip.h"
#include "combine_tools.h"
#include "ChunkStack.h"
#include "error_types.h"
#include "flag_parens.h"
#include "lang_pawn.h"
#include "language_tools.h"
#include "log_rules.h"
#include "newlines.h"
#include "prototypes.h"
#include "tokenize_cleanup.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>

using namespace std;
using namespace uncrustify;


chunk_t *fix_variable_definition(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc = start;
   chunk_t    *end;
   chunk_t    *tmp_pc;
   ChunkStack cs;
   int        idx;
   int        ref_idx;

   LOG_FMT(LFVD, "%s(%d): start at pc->orig_line is %zu, pc->orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);

   // Scan for words and types and stars oh my!
   while (  chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_QUALIFIER)
         || chunk_is_token(pc, CT_TYPENAME)
         || chunk_is_token(pc, CT_DC_MEMBER)
         || chunk_is_token(pc, CT_MEMBER)
         || chunk_is_ptr_operator(pc))
   {
      LOG_FMT(LFVD, "%s(%d):   1:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));
      cs.Push_Back(pc);
      pc = chunk_get_next_ncnl(pc);

      if (pc == nullptr)
      {
         LOG_FMT(LFVD, "%s(%d): pc is nullptr\n", __func__, __LINE__);
         return(nullptr);
      }
      LOG_FMT(LFVD, "%s(%d):   2:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      // Skip templates and attributes
      pc = skip_template_next(pc);

      if (pc == nullptr)
      {
         LOG_FMT(LFVD, "%s(%d): pc is nullptr\n", __func__, __LINE__);
         return(nullptr);
      }
      LOG_FMT(LFVD, "%s(%d):   3:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      pc = skip_attribute_next(pc);

      if (pc == nullptr)
      {
         LOG_FMT(LFVD, "%s(%d): pc is nullptr\n", __func__, __LINE__);
         return(nullptr);
      }
      LOG_FMT(LFVD, "%s(%d):   4:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      if (language_is_set(LANG_JAVA))
      {
         pc = skip_tsquare_next(pc);
         LOG_FMT(LFVD, "%s(%d):   5:pc->text() '%s', type is %s\n", __func__, __LINE__, pc->text(), get_token_name(pc->type));
      }
   }
   end = pc;

   if (end == nullptr)
   {
      LOG_FMT(LFVD, "%s(%d): end is nullptr\n", __func__, __LINE__);
      return(nullptr);
   }
   LOG_FMT(LFVD, "%s(%d): end->type is %s\n", __func__, __LINE__, get_token_name(end->type));

   if (  cs.Len() == 1
      && chunk_is_token(end, CT_BRACE_OPEN)
      && get_chunk_parent_type(end) == CT_BRACED_INIT_LIST)
   {
      set_chunk_type(cs.Get(0)->m_pc, CT_TYPE);
   }

   // Function defs are handled elsewhere
   if (  (cs.Len() <= 1)
      || chunk_is_token(end, CT_FUNC_DEF)
      || chunk_is_token(end, CT_FUNC_PROTO)
      || chunk_is_token(end, CT_FUNC_CLASS_DEF)
      || chunk_is_token(end, CT_FUNC_CLASS_PROTO)
      || chunk_is_token(end, CT_OPERATOR))
   {
      return(skip_to_next_statement(end));
   }
   // ref_idx points to the alignable part of the variable definition
   ref_idx = cs.Len() - 1;

   // Check for the '::' stuff: "char *Engine::name"
   if (  (cs.Len() >= 3)
      && (  (cs.Get(cs.Len() - 2)->m_pc->type == CT_MEMBER)
         || (cs.Get(cs.Len() - 2)->m_pc->type == CT_DC_MEMBER)))
   {
      idx = cs.Len() - 2;

      while (idx > 0)
      {
         tmp_pc = cs.Get(idx)->m_pc;

         if (  tmp_pc->type != CT_DC_MEMBER
            && tmp_pc->type != CT_MEMBER)
         {
            break;
         }

         if (idx == 0)
         {
            fprintf(stderr, "%s(%d): idx is ZERO, cannot be decremented, at line %zu, column %zu\n",
                    __func__, __LINE__, tmp_pc->orig_line, tmp_pc->orig_col);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         idx--;
         tmp_pc = cs.Get(idx)->m_pc;

         if (  tmp_pc->type != CT_WORD
            && tmp_pc->type != CT_TYPE)
         {
            break;
         }
         make_type(tmp_pc);
         idx--;
      }
      ref_idx = idx + 1;
   }
   tmp_pc = cs.Get(ref_idx)->m_pc;
   LOG_FMT(LFVD, "%s(%d): ref_idx(%d) is '%s'\n", __func__, __LINE__, ref_idx, tmp_pc->text());

   // No type part found!
   if (ref_idx <= 0)
   {
      return(skip_to_next_statement(end));
   }
   LOG_FMT(LFVD2, "%s(%d): orig_line is %zu, TYPE : ", __func__, __LINE__, start->orig_line);

   for (size_t idxForCs = 0; idxForCs < cs.Len() - 1; idxForCs++)
   {
      tmp_pc = cs.Get(idxForCs)->m_pc;
      make_type(tmp_pc);
      chunk_flags_set(tmp_pc, PCF_VAR_TYPE);
      LOG_FMT(LFVD2, " text() is '%s', type is %s", tmp_pc->text(), get_token_name(tmp_pc->type));
   }

   LOG_FMT(LFVD2, "\n");

   // OK we have two or more items, mark types up to the end.
   LOG_FMT(LFVD, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);
   mark_variable_definition(cs.Get(cs.Len() - 1)->m_pc);

   if (chunk_is_token(end, CT_COMMA))
   {
      return(chunk_get_next_ncnl(end));
   }
   return(skip_to_next_statement(end));
} // fix_variable_definition
