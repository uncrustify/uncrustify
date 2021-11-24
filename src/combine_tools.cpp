/**
 * @file combine_tools.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "combine_tools.h"

#include "unc_ctype.h"
#include "uncrustify.h"


bool can_be_full_param(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFPARAM, "%s(%d): start->text() is '%s', type is %s\n",
           __func__, __LINE__, start->text(), get_token_name(start->type));
   LOG_FMT(LFPARAM, "%s(%d): end->text()   is '%s', type is %s\n",
           __func__, __LINE__, end->text(), get_token_name(end->type));

   int     word_count     = 0;
   int     type_count     = 0;
   chunk_t *pc            = nullptr;
   chunk_t *first_word    = nullptr;
   bool    first_word_set = false;

   for (pc = start;
        pc != nullptr && pc != end;
        pc = chunk_get_next_ncnnl(pc, scope_e::PREPROC))
   {
      LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      if (  chunk_is_token(pc, CT_QUALIFIER)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_ENUM)
         || chunk_is_token(pc, CT_UNION)
         || chunk_is_token(pc, CT_TYPENAME))
      {
         LOG_FMT(LFPARAM, "%s(%d): <== %s! (yes)\n",
                 __func__, __LINE__, get_token_name(pc->type));
         return(true);
      }

      if (  chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_TYPE))
      {
         ++word_count;

         if (!first_word_set)
         {
            first_word     = pc;
            first_word_set = true;
         }

         if (chunk_is_token(pc, CT_TYPE))
         {
            ++type_count;
         }
      }
      else if (  chunk_is_token(pc, CT_MEMBER)
              || chunk_is_token(pc, CT_DC_MEMBER))
      {
         if (word_count > 0)
         {
            --word_count;
         }
      }
      else if (  pc != start
              && chunk_is_ptr_operator(pc))
      {
         // chunk is OK
      }
      else if (chunk_is_token(pc, CT_ASSIGN))
      {
         // chunk is OK (default values)
         break;
      }
      else if (chunk_is_token(pc, CT_ANGLE_OPEN))
      {
         LOG_FMT(LFPARAM, "%s(%d): <== template\n",
                 __func__, __LINE__);

         return(true);
      }
      else if (chunk_is_token(pc, CT_ELLIPSIS))
      {
         LOG_FMT(LFPARAM, "%s(%d): <== ellipsis\n",
                 __func__, __LINE__);

         return(true);
      }
      else if (  word_count == 0
              && chunk_is_token(pc, CT_PAREN_OPEN))
      {
         // Check for old-school func proto param '(type)'
         chunk_t *tmp1 = chunk_skip_to_match(pc, scope_e::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }
         chunk_t *tmp2 = chunk_get_next_ncnnl(tmp1, scope_e::PREPROC);

         if (tmp2 == nullptr)
         {
            return(false);
         }

         if (  chunk_is_token(tmp2, CT_COMMA)
            || chunk_is_paren_close(tmp2))
         {
            do
            {
               pc = chunk_get_next_ncnnl(pc, scope_e::PREPROC);

               if (pc == nullptr)
               {
                  return(false);
               }
               LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->text(), get_token_name(pc->type));
            } while (pc != tmp1);

            // reset some vars to allow [] after parens
            word_count = 1;
            type_count = 1;
         }
         else
         {
            LOG_FMT(LFPARAM, "%s(%d): <== '%s' not fcn type!\n",
                    __func__, __LINE__, get_token_name(pc->type));
            return(false);
         }
      }
      else if (  (  word_count == 1
                 || (word_count == type_count))
              && chunk_is_token(pc, CT_PAREN_OPEN))
      {
         // Check for func proto param 'void (*name)' or 'void (*name)(params)' or 'void (^name)(params)'
         // <name> can be optional
         chunk_t *tmp1 = chunk_get_next_ncnnl(pc, scope_e::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }
         chunk_t *tmp2 = chunk_get_next_ncnnl(tmp1, scope_e::PREPROC);

         if (tmp2 == nullptr)
         {
            return(false);
         }
         chunk_t *tmp3 = (chunk_is_str(tmp2, ")", 1)) ? tmp2 : chunk_get_next_ncnnl(tmp2, scope_e::PREPROC);

         if (tmp3 == nullptr)
         {
            return(false);
         }

         if (  !chunk_is_str(tmp3, ")", 1)
            || !(  chunk_is_str(tmp1, "*", 1)
                || chunk_is_str(tmp1, "^", 1)) // Issue #2656
            || !(  tmp2->type == CT_WORD
                || chunk_is_str(tmp2, ")", 1)))
         {
            LOG_FMT(LFPARAM, "%s(%d): <== '%s' not fcn type!\n",
                    __func__, __LINE__, get_token_name(pc->type));
            return(false);
         }
         LOG_FMT(LFPARAM, "%s(%d): <skip fcn type>\n",
                 __func__, __LINE__);

         tmp1 = chunk_get_next_ncnnl(tmp3, scope_e::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }

         if (chunk_is_str(tmp1, "(", 1))
         {
            tmp3 = chunk_skip_to_match(tmp1, scope_e::PREPROC);
         }
         pc = tmp3;
         LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type));

         // reset some vars to allow [] after parens
         word_count = 1;
         type_count = 1;
      }
      else if (chunk_is_token(pc, CT_TSQUARE))
      {
         // ignore it
      }
      else if (  word_count == 1
              && chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         // skip over any array stuff
         pc = chunk_skip_to_match(pc, scope_e::PREPROC);
         LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type));
      }
      else if (  word_count == 2
              && chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         // Bug #671: is it such as: bool foo[FOO_MAX]
         pc = chunk_skip_to_match(pc, scope_e::PREPROC);
         LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type));
      }
      else if (  word_count == 1
              && language_is_set(LANG_CPP)
              && chunk_is_str(pc, "&&", 2))
      {
         // ignore possible 'move' operator
      }
      else
      {
         LOG_FMT(LFPARAM, "%s(%d): <== type is %s, no way!, type count is %d, word count is %d\n",
                 __func__, __LINE__, get_token_name(pc->type), type_count, word_count);
         return(false);
      }
      LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));
   }

   chunk_t *last = chunk_get_prev_ncnnlni(pc);   // Issue #2279

   LOG_FMT(LFPARAM, "%s(%d): last->text() is '%s', type is %s\n",
           __func__, __LINE__, last->text(), get_token_name(last->type));

   if (chunk_is_ptr_operator(last))
   {
      LOG_FMT(LFPARAM, "%s(%d): <== type is %s, sure!\n",
              __func__, __LINE__, get_token_name(last->type));
      return(true);
   }

   if (  word_count < 2
      && type_count < 1
      && start->brace_level > 0)
   {
      LOG_FMT(LFPARAM, "%s(%d): !MVP!\n",
              __func__, __LINE__);
      // Oh, joy, we are in Most Vexing Parse territory
      auto const brace =
         chunk_get_prev_type(start, CT_BRACE_OPEN, start->brace_level - 1);

      if (brace != nullptr)
      {
         LOG_FMT(LFPARAM, "%s(%d): (matching %s brace at orig_line %zu, orig_col is %zu)",
                 __func__, __LINE__,
                 get_token_name(get_chunk_parent_type(brace)), brace->orig_line, brace->orig_col);
      }

      if (  brace != nullptr
         && (  get_chunk_parent_type(brace) == CT_CLASS
            || get_chunk_parent_type(brace) == CT_STRUCT))
      {
         // A Most Vexing Parse variable declaration cannot occur in the body
         // of a struct/class, so we probably have a function prototype
         LOG_FMT(LFPARAM, "%s(%d): <== type is %s, Likely!\n",
                 __func__, __LINE__, (pc == nullptr ? "nullptr" : get_token_name(pc->type)));
         return(true);
      }
   }
   LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', word_count is %d, type_count is %d\n",
           __func__, __LINE__, pc->text(), word_count, type_count);

   if (first_word != nullptr)
   {
      LOG_FMT(LFPARAM, "%s(%d): first_word->text() is '%s'\n",
              __func__, __LINE__, first_word->text());
   }
   bool ret = (  word_count >= 2
              || (  word_count == 1
                 && type_count == 1));

   LOG_FMT(LFPARAM, "%s(%d): ret is %s\n",
           __func__, __LINE__, ret ? "TRUE" : "FALSE");

   LOG_FMT(LFPARAM, "%s(%d): pc->text() is '%s', ",
           __func__, __LINE__, pc->text());
   LOG_FMT(LFPARAM, "<== type is %s, ",
           (pc == nullptr ? "nullptr" : get_token_name(pc->type)));

   if (ret)
   {
      LOG_FMT(LFPARAM, "Yup!\n");
   }
   else
   {
      LOG_FMT(LFPARAM, "Unlikely!\n");
   }
   return(ret);
} // can_be_full_param


bool chunk_ends_type(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc       = start;
   bool    ret       = false;
   size_t  cnt       = 0;
   bool    last_expr = false;
   bool    last_lval = false;

   bool    a = pc->flags.test(PCF_IN_FCN_CTOR);

   if (a)
   {
      return(false);
   }

   for ( ; pc != nullptr; pc = chunk_get_prev_ncnnlni(pc)) // Issue #2279
   {
      LOG_FMT(LFTYPE, "%s(%d): type is %s, text() '%s', orig_line %zu, orig_col %zu\n   ",
              __func__, __LINE__, get_token_name(pc->type), pc->text(),
              pc->orig_line, pc->orig_col);
      log_pcf_flags(LFTYPE, pc->flags);

      if (  chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_PTR_TYPE)
         || chunk_is_token(pc, CT_STAR)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_DC_MEMBER)
         || chunk_is_token(pc, CT_PP)
         || chunk_is_token(pc, CT_QUALIFIER)
         || (  language_is_set(LANG_CPP | LANG_OC)                       // Issue #2727
            && get_chunk_parent_type(pc) == CT_TEMPLATE
            && (  chunk_is_token(pc, CT_ANGLE_OPEN)
               || chunk_is_token(pc, CT_ANGLE_CLOSE)))
         || (  language_is_set(LANG_CS)
            && (chunk_is_token(pc, CT_MEMBER))))
      {
         cnt++;
         last_expr = pc->flags.test(PCF_EXPR_START)
                     && !pc->flags.test(PCF_IN_FCN_CALL);
         last_lval = pc->flags.test(PCF_LVALUE);
         continue;
      }
      /* If a comma is encountered within a template, it must be
       * considered within the context of its immediate parent
       * template (i.e. argument list nest level)
       */

      if (  (  chunk_is_semicolon(pc)
            && !pc->flags.test(PCF_IN_FOR))
         || chunk_is_token(pc, CT_TYPEDEF)
         || chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_VBRACE_CLOSE)
         || chunk_is_token(pc, CT_FPAREN_CLOSE)
         || chunk_is_forin(pc)
         || chunk_is_token(pc, CT_MACRO)
         || chunk_is_token(pc, CT_PP_IF)
         || chunk_is_token(pc, CT_PP_ELSE)
         || chunk_is_token(pc, CT_PP_ENDIF)
         || get_chunk_parent_type(pc) == CT_PP_INCLUDE                       // Issue #3233
         || (  (  chunk_is_token(pc, CT_COMMA)
               && !pc->flags.test(PCF_IN_FCN_CALL)
               && get_cpp_template_angle_nest_level(start) ==
                  get_cpp_template_angle_nest_level(pc))
            && last_expr)
         || (  chunk_is_token(pc, CT_SPAREN_OPEN)
            && last_lval))
      {
         ret = cnt > 0;
      }
      break;
   }

   if (pc == nullptr)
   {
      // first token
      ret = true;
   }
   LOG_FMT(LFTYPE, "%s(%d): first token verdict: %s\n",
           __func__, __LINE__, ret ? "yes" : "no");

   return(ret);
} // chunk_ends_type


bool chunkstack_match(ChunkStack &cs, chunk_t *pc)
{
   for (size_t idx = 0; idx < cs.Len(); idx++)
   {
      chunk_t *tmp = cs.GetChunk(idx);

      if (pc->str.equals(tmp->str))
      {
         return(true);
      }
   }

   return(false);
} // chunkstack_match


void flag_series(chunk_t *start, chunk_t *end, pcf_flags_t set_flags, pcf_flags_t clr_flags, scope_e nav)
{
   LOG_FUNC_ENTRY();

   while (  start != nullptr
         && start != end)
   {
      chunk_flags_upd(start, clr_flags, set_flags);

      start = chunk_get_next(start, nav);

      if (start == nullptr)
      {
         return;
      }
   }

   if (end != nullptr)
   {
      chunk_flags_upd(end, clr_flags, set_flags);
   }
} // flag_series


size_t get_cpp_template_angle_nest_level(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   int nestLevel = 0;

   while (  pc != nullptr
         && pc->flags.test(PCF_IN_TEMPLATE))
   {
      if (  chunk_is_token(pc, CT_ANGLE_CLOSE)
         && get_chunk_parent_type(pc) == CT_TEMPLATE)
      {
         --nestLevel;
      }
      else if (  chunk_is_token(pc, CT_ANGLE_OPEN)
              && get_chunk_parent_type(pc) == CT_TEMPLATE)
      {
         ++nestLevel;
      }
      pc = chunk_get_prev_ncnnlni(pc);
   }
   return(nestLevel <= 0 ? 0 : size_t(nestLevel));
}


chunk_t *get_d_template_types(ChunkStack &cs, chunk_t *open_paren)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp       = open_paren;
   bool    maybe_type = true;

   while (  ((tmp = chunk_get_next_ncnnl(tmp)) != nullptr)
         && tmp->level > open_paren->level)
   {
      if (  chunk_is_token(tmp, CT_TYPE)
         || chunk_is_token(tmp, CT_WORD))
      {
         if (maybe_type)
         {
            make_type(tmp);
            cs.Push_Back(tmp);
         }
         maybe_type = false;
      }
      else if (chunk_is_token(tmp, CT_COMMA))
      {
         maybe_type = true;
      }
   }
   return(tmp);
} // get_d_template_types


bool go_on(chunk_t *pc, chunk_t *start)
{
   if (  pc == nullptr
      || pc->level != start->level)
   {
      return(false);
   }

   if (pc->flags.test(PCF_IN_FOR))
   {
      return(  (!chunk_is_semicolon(pc))
            && (!(chunk_is_token(pc, CT_COLON))));
   }
   return(!chunk_is_semicolon(pc));
} // go_on


bool is_ucase_str(const char *str, size_t len)
{
   while (len-- > 0)
   {
      if (unc_toupper(*str) != *str)
      {
         return(false);
      }
      str++;
   }
   return(true);
} // is_ucase_str


void make_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_WORD))
      {
         set_chunk_type(pc, CT_TYPE);
      }
      else if (  (  chunk_is_star(pc)
                 || chunk_is_msref(pc)
                 || chunk_is_nullable(pc))
              && chunk_is_type(pc->prev))                              // Issue # 2640
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (  chunk_is_addr(pc)
              && !chunk_is_token(pc->prev, CT_SQUARE_OPEN))            // Issue # 2166
      {
         set_chunk_type(pc, CT_BYREF);
      }
   }
} // make_type


chunk_t *set_paren_parent(chunk_t *start, c_token_t parent)
{
   LOG_FUNC_ENTRY();
   chunk_t *end;

   end = chunk_skip_to_match(start, scope_e::PREPROC);

   if (end != nullptr)
   {
      LOG_FMT(LFLPAREN, "%s(%d): %zu:%zu '%s' and %zu:%zu '%s' type is %s, parent_type is %s",
              __func__, __LINE__, start->orig_line, start->orig_col, start->text(),
              end->orig_line, end->orig_col, end->text(),
              get_token_name(start->type), get_token_name(parent));
      log_func_stack_inline(LFLPAREN);
      set_chunk_parent(start, parent);
      set_chunk_parent(end, parent);
   }
   LOG_FMT(LFLPAREN, "%s(%d):\n", __func__, __LINE__);
   return(chunk_get_next_ncnnl(end, scope_e::PREPROC));
} // set_paren_parent
