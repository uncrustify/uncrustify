/**
 * @file combine_mark.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "combine_mark.h"

#include "chunk_list.h"
#include "combine_fix.h"
#include "combine_skip.h"
#include "combine_tools.h"
#include "flag_parens.h"
#include "uncrustify.h"


void mark_lvalue(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev;

   if (pc->flags.test(PCF_IN_PREPROC))
   {
      return;
   }

   for (prev = chunk_get_prev_ncnlni(pc);     // Issue #2279
        prev != nullptr;
        prev = chunk_get_prev_ncnlni(prev))   // Issue #2279
   {
      if (  prev->level < pc->level
         || chunk_is_token(prev, CT_ASSIGN)
         || chunk_is_token(prev, CT_COMMA)
         || chunk_is_token(prev, CT_BOOL)
         || chunk_is_semicolon(prev)
         || chunk_is_str(prev, "(", 1)
         || chunk_is_str(prev, "{", 1)
         || chunk_is_str(prev, "[", 1)
         || prev->flags.test(PCF_IN_PREPROC))
      {
         break;
      }
      chunk_flags_set(prev, PCF_LVALUE);

      if (prev->level == pc->level && chunk_is_str(prev, "&", 1))
      {
         make_type(prev);
      }
   }
}


void mark_function_return_type(chunk_t *fname, chunk_t *start, c_token_t parent_type)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   if (pc != nullptr)
   {
      // Step backwards from pc and mark the parent of the return type
      LOG_FMT(LFCNR, "%s(%d): (backwards) return type for '%s' @ orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, fname->text(), fname->orig_line, fname->orig_col);

      chunk_t *first = pc;

      while (pc != nullptr)
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s, ",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         log_pcf_flags(LFCNR, pc->flags);

         if (chunk_is_token(pc, CT_ANGLE_CLOSE))
         {
            pc = skip_template_prev(pc);

            if (pc == nullptr || chunk_is_token(pc, CT_TEMPLATE))
            {
               //either expression is not complete or this is smth like 'template<T> void func()'
               //  - we are not interested in 'template<T>' part
               break;
            }
            else
            {
               //this is smth like 'vector<int> func()' and 'pc' is currently on 'vector' - just proceed
            }
         }

         if (  (  !chunk_is_type(pc)
               && pc->type != CT_OPERATOR
               && pc->type != CT_WORD
               && pc->type != CT_ADDR)
            || pc->flags.test(PCF_IN_PREPROC))
         {
            break;
         }

         if (!chunk_is_ptr_operator(pc))
         {
            first = pc;
         }
         pc = chunk_get_prev_ncnlni(pc);   // Issue #2279
      }
      LOG_FMT(LFCNR, "%s(%d): marking returns...", __func__, __LINE__);

      // Changing words to types into tuple return types in CS.
      bool is_return_tuple = false;

      if (chunk_is_token(pc, CT_PAREN_CLOSE) && !pc->flags.test(PCF_IN_PREPROC))
      {
         first           = chunk_skip_to_match_rev(pc);
         is_return_tuple = true;
      }
      pc = first;

      while (pc != nullptr)
      {
         LOG_FMT(LFCNR, " text() '%s', type is %s", pc->text(), get_token_name(pc->type));

         if (parent_type != CT_NONE)
         {
            set_chunk_parent(pc, parent_type);
         }
         chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279

         if (  !is_return_tuple
            || pc->type != CT_WORD
            || (prev != nullptr && prev->type != CT_TYPE))
         {
            make_type(pc);
         }

         if (pc == start)
         {
            break;
         }
         pc = chunk_get_next_ncnl(pc);

         //template angles should keep parent type CT_TEMPLATE
         if (chunk_is_token(pc, CT_ANGLE_OPEN))
         {
            pc = chunk_get_next_type(pc, CT_ANGLE_CLOSE, pc->level);

            if (pc == start)
            {
               break;
            }
            pc = chunk_get_next_ncnl(pc);
         }
      }
      LOG_FMT(LFCNR, "\n");

      // Back up and mark parent type on friend declarations
      if (parent_type != CT_NONE && first && first->flags.test(PCF_IN_CLASS))
      {
         pc = chunk_get_prev_ncnlni(first);   // Issue #2279

         if (chunk_is_token(pc, CT_FRIEND))
         {
            LOG_FMT(LFCNR, "%s(%d): marking friend\n", __func__, __LINE__);
            set_chunk_parent(pc, parent_type);
            // A friend might be preceded by a template specification, as in:
            //   template <...> friend type func(...);
            // If so, we need to mark that also
            pc = chunk_get_prev_ncnlni(pc);   // Issue #2279

            if (chunk_is_token(pc, CT_ANGLE_CLOSE))
            {
               pc = skip_template_prev(pc);

               if (chunk_is_token(pc, CT_TEMPLATE))
               {
                  LOG_FMT(LFCNR, "%s(%d): marking friend template\n",
                          __func__, __LINE__);
                  set_chunk_parent(pc, parent_type);
               }
            }
         }
      }
   }
} // mark_function_return_type


bool mark_function_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTYPE, "%s(%d): type is %s, text() '%s' @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->text(),
           pc->orig_line, pc->orig_col);

   size_t    star_count = 0;
   size_t    word_count = 0;
   chunk_t   *ptrcnk    = nullptr;
   chunk_t   *tmp;
   chunk_t   *apo;
   chunk_t   *apc;
   chunk_t   *aft;
   bool      anon = false;
   c_token_t pt, ptp;

   // Scan backwards across the name, which can only be a word and single star
   chunk_t *varcnk = chunk_get_prev_ncnlni(pc);   // Issue #2279

   varcnk = chunk_get_prev_ssq(varcnk);

   if (varcnk != nullptr && !chunk_is_word(varcnk))
   {
      if (  language_is_set(LANG_OC)
         && chunk_is_str(varcnk, "^", 1)
         && chunk_is_paren_open(chunk_get_prev_ncnlni(varcnk)))   // Issue #2279
      {
         // anonymous ObjC block type -- RTYPE (^)(ARGS)
         anon = true;
      }
      else
      {
         LOG_FMT(LFTYPE, "%s(%d): not a word: text() '%s', type is %s, @ orig_line is %zu:, orig_col is %zu\n",
                 __func__, __LINE__, varcnk->text(), get_token_name(varcnk->type),
                 varcnk->orig_line, varcnk->orig_col);
         goto nogo_exit;
      }
   }
   apo = chunk_get_next_ncnl(pc);

   if (apo == nullptr)
   {
      return(false);
   }
   apc = chunk_skip_to_match(apo);

   if (  apc != nullptr
      && (  !chunk_is_paren_open(apo)
         || ((apc = chunk_skip_to_match(apo)) == nullptr)))
   {
      LOG_FMT(LFTYPE, "%s(%d): not followed by parens\n", __func__, __LINE__);
      goto nogo_exit;
   }
   aft = chunk_get_next_ncnl(apc);

   if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      pt = CT_FUNC_DEF;
   }
   else if (chunk_is_token(aft, CT_SEMICOLON) || chunk_is_token(aft, CT_ASSIGN))
   {
      pt = CT_FUNC_PROTO;
   }
   else
   {
      LOG_FMT(LFTYPE, "%s(%d): not followed by '{' or ';'\n", __func__, __LINE__);
      goto nogo_exit;
   }
   ptp = pc->flags.test(PCF_IN_TYPEDEF) ? CT_FUNC_TYPE : CT_FUNC_VAR;

   tmp = pc;

   while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
   {
      tmp = chunk_get_prev_ssq(tmp);

      LOG_FMT(LFTYPE, " -- type is %s, %s on orig_line %zu, orig_col is %zu",
              get_token_name(tmp->type), tmp->text(),
              tmp->orig_line, tmp->orig_col);

      if (  chunk_is_star(tmp)
         || chunk_is_token(tmp, CT_PTR_TYPE)
         || chunk_is_token(tmp, CT_CARET))
      {
         star_count++;
         ptrcnk = tmp;
         LOG_FMT(LFTYPE, " -- PTR_TYPE\n");
      }
      else if (  chunk_is_word(tmp)
              || chunk_is_token(tmp, CT_WORD)
              || chunk_is_token(tmp, CT_TYPE))
      {
         word_count++;
         LOG_FMT(LFTYPE, " -- TYPE(%s)\n", tmp->text());
      }
      else if (chunk_is_token(tmp, CT_DC_MEMBER))
      {
         word_count = 0;
         LOG_FMT(LFTYPE, " -- :: reset word_count\n");
      }
      else if (chunk_is_str(tmp, "(", 1))
      {
         LOG_FMT(LFTYPE, " -- open paren (break)\n");
         break;
      }
      else
      {
         LOG_FMT(LFTYPE, " --  unexpected token: type is %s, text() '%s', on orig_line %zu, orig_col %zu\n",
                 get_token_name(tmp->type), tmp->text(),
                 tmp->orig_line, tmp->orig_col);
         goto nogo_exit;
      }
   }

   // Fixes #issue 1577
   // Allow word count 2 incase of function pointer declaration.
   // Ex: bool (__stdcall* funcptr)(int, int);
   if (  star_count > 1
      || (word_count > 1 && !(word_count == 2 && ptp == CT_FUNC_VAR))
      || ((star_count + word_count) == 0))
   {
      LOG_FMT(LFTYPE, "%s(%d): bad counts word: %zu, star: %zu\n",
              __func__, __LINE__, word_count, star_count);
      goto nogo_exit;
   }

   // make sure what appears before the first open paren can be a return type
   if (!chunk_ends_type(chunk_get_prev_ncnlni(tmp)))   // Issue #2279
   {
      goto nogo_exit;
   }

   if (ptrcnk)
   {
      set_chunk_type(ptrcnk, CT_PTR_TYPE);
   }

   if (!anon)
   {
      if (pc->flags.test(PCF_IN_TYPEDEF))
      {
         set_chunk_type(varcnk, CT_TYPE);
      }
      else
      {
         set_chunk_type(varcnk, CT_FUNC_VAR);
         chunk_flags_set(varcnk, PCF_VAR_1ST_DEF);
      }
   }
   set_chunk_type(pc, CT_TPAREN_CLOSE);
   set_chunk_parent(pc, ptp);

   set_chunk_type(apo, CT_FPAREN_OPEN);
   set_chunk_parent(apo, pt);
   set_chunk_type(apc, CT_FPAREN_CLOSE);
   set_chunk_parent(apc, pt);
   fix_fcn_def_params(apo);

   if (chunk_is_semicolon(aft))
   {
      set_chunk_parent(aft, aft->flags.test(PCF_IN_TYPEDEF) ? CT_TYPEDEF : CT_FUNC_VAR);
   }
   else if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      flag_parens(aft, PCF_NONE, CT_NONE, pt, false);
   }
   // Step backwards to the previous open paren and mark everything a
   tmp = pc;

   while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
   {
      LOG_FMT(LFTYPE, " ++ type is %s, text() '%s', on orig_line %zu, orig_col %zu\n",
              get_token_name(tmp->type), tmp->text(),
              tmp->orig_line, tmp->orig_col);

      if (*tmp->str.c_str() == '(')
      {
         if (!pc->flags.test(PCF_IN_TYPEDEF))
         {
            chunk_flags_set(tmp, PCF_VAR_1ST_DEF);
         }
         set_chunk_type(tmp, CT_TPAREN_OPEN);
         set_chunk_parent(tmp, ptp);

         tmp = chunk_get_prev_ncnlni(tmp);   // Issue #2279

         if (  chunk_is_token(tmp, CT_FUNCTION)
            || chunk_is_token(tmp, CT_FUNC_CALL)
            || chunk_is_token(tmp, CT_FUNC_CALL_USER)
            || chunk_is_token(tmp, CT_FUNC_DEF)
            || chunk_is_token(tmp, CT_FUNC_PROTO))
         {
            set_chunk_type(tmp, CT_TYPE);
            chunk_flags_clr(tmp, PCF_VAR_1ST_DEF);
         }
         mark_function_return_type(varcnk, tmp, ptp);
         break;
      }
   }
   return(true);

nogo_exit:
   tmp = chunk_get_next_ncnl(pc);

   if (chunk_is_paren_open(tmp))
   {
      LOG_FMT(LFTYPE, "%s(%d): setting FUNC_CALL on orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col);
      flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
   }
   return(false);
} // mark_function_type


void mark_variable_stack(ChunkStack &cs, log_sev_t sev)
{
   UNUSED(sev);
   LOG_FUNC_ENTRY();

   // throw out the last word and mark the rest
   chunk_t *var_name = cs.Pop_Back();

   if (var_name && var_name->prev->type == CT_DC_MEMBER)
   {
      cs.Push_Back(var_name);
   }

   if (var_name != nullptr)
   {
      LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu:\n",
              __func__, __LINE__, var_name->orig_line, var_name->orig_col);

      size_t  word_cnt = 0;
      chunk_t *word_type;

      while ((word_type = cs.Pop_Back()) != nullptr)
      {
         if (chunk_is_token(word_type, CT_WORD) || chunk_is_token(word_type, CT_TYPE))
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, word_type->text());
            set_chunk_type(word_type, CT_TYPE);
            chunk_flags_set(word_type, PCF_VAR_TYPE);
         }
         word_cnt++;
      }

      if (chunk_is_token(var_name, CT_WORD))
      {
         if (word_cnt > 0)
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as VAR\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, var_name->text());
            chunk_flags_set(var_name, PCF_VAR_DEF);
         }
         else
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, var_name->text());
            set_chunk_type(var_name, CT_TYPE);
            chunk_flags_set(var_name, PCF_VAR_TYPE);
         }
      }
   }
} // mark_variable_stack


chunk_t *mark_variable_definition(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return(nullptr);
   }
   chunk_t     *pc   = start;
   pcf_flags_t flags = PCF_VAR_1ST_DEF;

   LOG_FMT(LVARDEF, "%s(%d): orig_line %zu, orig_col %zu, text() '%s', type is %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
           get_token_name(pc->type));

   // Issue #596
   bool bit_field_colon_is_present = false;

   while (go_on(pc, start))
   {
      if (  chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_FUNC_CTOR_VAR))
      {
         auto const orig_flags = pc->flags;

         if (!pc->flags.test(PCF_IN_ENUM))
         {
            chunk_flags_set(pc, flags);
         }
         flags &= ~PCF_VAR_1ST;
         LOG_FMT(LVARDEF, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', set PCF_VAR_1ST\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

         LOG_FMT(LVARDEF,
                 "%s(%d): orig_line is %zu, marked text() '%s'[%s] "
                 "in orig_col %zu, flags: %s -> %s\n",
                 __func__, __LINE__, pc->orig_line, pc->text(),
                 get_token_name(pc->type), pc->orig_col,
                 pcf_flags_str(orig_flags).c_str(),
                 pcf_flags_str(pc->flags).c_str());
      }
      else if (  !bit_field_colon_is_present                      // Issue #2689
              && (  chunk_is_star(pc)
                 || chunk_is_msref(pc)))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_addr(pc))
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else if (  chunk_is_token(pc, CT_SQUARE_OPEN)
              || chunk_is_token(pc, CT_ASSIGN))
      {
         pc = skip_expression(pc);
         continue;
      }
      else if (chunk_is_token(pc, CT_COLON))
      {
         bit_field_colon_is_present = true;                    // Issue #2689
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
} // mark_variable_definition


void mark_function(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
   chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279
   chunk_t *next = chunk_get_next_ncnlnp(pc);

   if (next == nullptr)
   {
      return;
   }
   chunk_t *tmp;
   chunk_t *semi = nullptr;
   chunk_t *paren_open;
   chunk_t *paren_close;

   // Find out what is before the operator
   if (get_chunk_parent_type(pc) == CT_OPERATOR)
   {
      LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      log_pcf_flags(LGUY, pc->flags);
      chunk_t *pc_op = chunk_get_prev_type(pc, CT_OPERATOR, pc->level);

      if (  pc_op != nullptr
         && pc_op->flags.test(PCF_EXPR_START))
      {
         LOG_FMT(LFCN, "%s(%d): (4) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         set_chunk_type(pc, CT_FUNC_CALL);
      }

      if (language_is_set(LANG_CPP))
      {
         tmp = pc;

         while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
         {
            if (  chunk_is_token(tmp, CT_BRACE_CLOSE)
               || chunk_is_token(tmp, CT_BRACE_OPEN)             // Issue 575
               || chunk_is_token(tmp, CT_SEMICOLON))
            {
               break;
            }

            if (chunk_is_paren_open(tmp))
            {
               LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());
               LOG_FMT(LFCN, "%s(%d): (5) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }

            if (chunk_is_token(tmp, CT_ASSIGN))
            {
               LOG_FMT(LFCN, "%s(%d): (6) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }

            if (chunk_is_token(tmp, CT_TEMPLATE))
            {
               LOG_FMT(LFCN, "%s(%d): (7) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
               set_chunk_type(pc, CT_FUNC_DEF);
               break;
            }

            if (chunk_is_token(tmp, CT_BRACE_OPEN))
            {
               if (get_chunk_parent_type(tmp) == CT_FUNC_DEF)
               {
                  LOG_FMT(LFCN, "%s(%d): (8) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
                  set_chunk_type(pc, CT_FUNC_CALL);
               }

               if (  get_chunk_parent_type(tmp) == CT_CLASS
                  || get_chunk_parent_type(tmp) == CT_STRUCT)
               {
                  LOG_FMT(LFCN, "%s(%d): (9) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
                  set_chunk_type(pc, CT_FUNC_DEF);
               }
               break;
            }
         }

         if (  tmp != nullptr
            && pc->type != CT_FUNC_CALL)
         {
            // Mark the return type
            while (  (tmp = chunk_get_next_ncnl(tmp)) != pc
                  && tmp != nullptr)
            {
               make_type(tmp); // Mark the return type
            }
         }
      }
   }

   if (chunk_is_ptr_operator(next))
   {
      next = chunk_get_next_ncnlnp(next);

      if (next == nullptr)
      {
         return;
      }
   }
   LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s, type is %s, parent_type is %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)));
   LOG_FMT(LFCN, "   level is %zu, brace_level is %zu, next->text() '%s', next->type is %s, next->level is %zu\n",
           pc->level, pc->brace_level,
           next->text(), get_token_name(next->type), next->level);

   if (pc->flags.test(PCF_IN_CONST_ARGS))
   {
      set_chunk_type(pc, CT_FUNC_CTOR_VAR);
      LOG_FMT(LFCN, "%s(%d):   1) Marked [%s] as FUNC_CTOR_VAR on line %zu col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      next = skip_template_next(next);

      if (next == nullptr)
      {
         return;
      }
      flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, pc->type, true);
      return;
   }
   // Skip over any template and attribute madness
   next = skip_template_next(next);

   if (next == nullptr)
   {
      return;
   }
   next = skip_attribute_next(next);

   if (next == nullptr)
   {
      return;
   }
   // Find the open and close parenthesis
   paren_open  = chunk_get_next_str(pc, "(", 1, pc->level);
   paren_close = chunk_get_next_str(paren_open, ")", 1, pc->level);

   if (  paren_open == nullptr
      || paren_close == nullptr)
   {
      LOG_FMT(LFCN, "%s(%d): No parens found for [%s] on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      return;
   }
   /*
    * This part detects either chained function calls or a function ptr definition.
    * MYTYPE (*func)(void);
    * mWriter( "class Clst_"c )( somestr.getText() )( " : Cluster {"c ).newline;
    *
    * For it to be a function variable def, there must be a '*' followed by a
    * single word.
    *
    * Otherwise, it must be chained function calls.
    */
   tmp = chunk_get_next_ncnl(paren_close);

   if (  tmp != nullptr
      && chunk_is_str(tmp, "(", 1))
   {
      chunk_t *tmp1;
      chunk_t *tmp2;
      chunk_t *tmp3;

      // skip over any leading class/namespace in: "T(F::*A)();"
      tmp1 = chunk_get_next_ncnl(next);

      while (tmp1 != nullptr)
      {
         tmp2 = chunk_get_next_ncnl(tmp1);

         if (!chunk_is_word(tmp1) || !chunk_is_token(tmp2, CT_DC_MEMBER))
         {
            break;
         }
         tmp1 = chunk_get_next_ncnl(tmp2);
      }
      tmp2 = chunk_get_next_ncnl(tmp1);

      if (chunk_is_str(tmp2, ")", 1))
      {
         tmp3 = tmp2;
         tmp2 = nullptr;
      }
      else
      {
         tmp3 = chunk_get_next_ncnl(tmp2);
      }
      tmp3 = chunk_get_next_ssq(tmp3);

      if (  chunk_is_str(tmp3, ")", 1)
         && (  chunk_is_star(tmp1)
            || chunk_is_msref(tmp1)
            || (language_is_set(LANG_OC) && chunk_is_token(tmp1, CT_CARET)))
         && (tmp2 == nullptr || chunk_is_token(tmp2, CT_WORD)))
      {
         if (tmp2)
         {
            LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, function variable '%s', changing '%s' into a type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, tmp2->text(), pc->text());
            set_chunk_type(tmp2, CT_FUNC_VAR);
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_VAR, false);

            LOG_FMT(LFCN, "%s(%d): paren open @ orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, paren_open->orig_line, paren_open->orig_col);
         }
         else
         {
            LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, function type, changing '%s' into a type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

            if (tmp2)
            {
               set_chunk_type(tmp2, CT_FUNC_TYPE);
            }
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_TYPE, false);
         }
         set_chunk_type(pc, CT_TYPE);
         set_chunk_type(tmp1, CT_PTR_TYPE);
         chunk_flags_clr(pc, PCF_VAR_1ST_DEF);

         if (tmp2 != nullptr)
         {
            chunk_flags_set(tmp2, PCF_VAR_1ST_DEF);
         }
         flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_PROTO, false);
         fix_fcn_def_params(tmp);
         return;
      }
      LOG_FMT(LFCN, "%s(%d): chained function calls? text() is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   }

   // Assume it is a function call if not already labeled
   if (chunk_is_token(pc, CT_FUNCTION))
   {
      LOG_FMT(LFCN, "%s(%d): examine: text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
      // look for an assigment. Issue #575
      chunk_t *temp = chunk_get_next_type(pc, CT_ASSIGN, pc->level);

      if (temp != nullptr)
      {
         LOG_FMT(LFCN, "%s(%d): assigment found, orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, temp->orig_line, temp->orig_col, temp->text());
         LOG_FMT(LFCN, "%s(%d): (10) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): (11) SET TO %s: orig_line is %zu, orig_col is %zu, text() '%s'",
                 __func__, __LINE__, (get_chunk_parent_type(pc) == CT_OPERATOR) ? "CT_FUNC_DEF" : "CT_FUNC_CALL",
                 pc->orig_line, pc->orig_col, pc->text());
         set_chunk_type(pc, (get_chunk_parent_type(pc) == CT_OPERATOR) ? CT_FUNC_DEF : CT_FUNC_CALL);
      }
   }
   LOG_FMT(LFCN, "%s(%d): Check for C++ function def, text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
           __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));

   if (prev != nullptr)
   {
      LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));
   }

   // Check for C++ function def
   if (  chunk_is_token(pc, CT_FUNC_CLASS_DEF)
      || (  prev != nullptr
         && (  chunk_is_token(prev, CT_INV)
            || chunk_is_token(prev, CT_DC_MEMBER))))
   {
      chunk_t *destr = nullptr;

      if (chunk_is_token(prev, CT_INV))
      {
         // TODO: do we care that this is the destructor?
         set_chunk_type(prev, CT_DESTRUCTOR);
         set_chunk_type(pc, CT_FUNC_CLASS_DEF);

         set_chunk_parent(pc, CT_DESTRUCTOR);

         destr = prev;
         // Point to the item previous to the class name
         prev = chunk_get_prev_ncnlnp(prev);
      }

      if (chunk_is_token(prev, CT_DC_MEMBER))
      {
         prev = chunk_get_prev_ncnlnp(prev);
         LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                 get_token_name(prev->type));
         prev = skip_template_prev(prev);
         LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                 get_token_name(prev->type));
         prev = skip_attribute_prev(prev);
         LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                 get_token_name(prev->type));

         if (chunk_is_token(prev, CT_WORD) || chunk_is_token(prev, CT_TYPE))
         {
            if (pc->str.equals(prev->str))
            {
               LOG_FMT(LFCN, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col,
                       get_token_name(prev->type));
               set_chunk_type(pc, CT_FUNC_CLASS_DEF);
               LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu - FOUND %sSTRUCTOR for '%s', type is %s\n",
                       __func__, __LINE__,
                       prev->orig_line, prev->orig_col,
                       (destr != nullptr) ? "DE" : "CON",
                       prev->text(), get_token_name(prev->type));

               mark_cpp_constructor(pc);
               return;
            }
            // Point to the item previous to the class name
            prev = chunk_get_prev_ncnlnp(prev);
         }
      }
   }

   /*
    * Determine if this is a function call or a function def/proto
    * We check for level==1 to allow the case that a function prototype is
    * wrapped in a macro: "MACRO(void foo(void));"
    */
   if (  chunk_is_token(pc, CT_FUNC_CALL)
      && (  pc->level == pc->brace_level
         || pc->level == 1)
      && !pc->flags.test(PCF_IN_ARRAY_ASSIGN))
   {
      bool isa_def  = false;
      bool hit_star = false;
      LOG_FMT(LFCN, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col,
              get_token_name(pc->type));

      if (prev == nullptr)
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev is NULL\n",
                 __func__, __LINE__);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev->text() '%s', prev->type is %s\n",
                 __func__, __LINE__, prev->text(), get_token_name(prev->type));
      }
      // if (!chunk_ends_type(prev))
      // {
      //    goto bad_ret_type;
      // }

      /*
       * REVISIT:
       * a function def can only occur at brace level, but not inside an
       * assignment, structure, enum, or union.
       * The close paren must be followed by an open brace, with an optional
       * qualifier (const) in between.
       * There can be all sorts of template stuff and/or '[]' in the type.
       * This hack mostly checks that.
       *
       * Examples:
       * foo->bar(maid);                   -- fcn call
       * FOO * bar();                      -- fcn proto or class variable
       * FOO foo();                        -- fcn proto or class variable
       * FOO foo(1);                       -- class variable
       * a = FOO * bar();                  -- fcn call
       * a.y = foo() * bar();              -- fcn call
       * static const char * const fizz(); -- fcn def
       */
      while (prev != nullptr)
      {
         LOG_FMT(LFCN, "%s(%d): next step with: prev->orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());

         if (get_chunk_parent_type(pc) == CT_FIXED)
         {
            isa_def = true;
         }

         if (prev->flags.test(PCF_IN_PREPROC))
         {
            prev = chunk_get_prev_ncnlnp(prev);
            continue;
         }

         // Some code slips an attribute between the type and function
         if (  chunk_is_token(prev, CT_FPAREN_CLOSE)
            && get_chunk_parent_type(prev) == CT_ATTRIBUTE)
         {
            prev = skip_attribute_prev(prev);
            continue;
         }

         // skip const(TYPE)
         if (  chunk_is_token(prev, CT_PAREN_CLOSE)
            && get_chunk_parent_type(prev) == CT_D_CAST)
         {
            LOG_FMT(LFCN, "%s(%d): --> For sure a prototype or definition\n",
                    __func__, __LINE__);
            isa_def = true;
            break;
         }

         if (get_chunk_parent_type(prev) == CT_DECLSPEC)  // Issue 1289
         {
            prev = chunk_skip_to_match_rev(prev);
            prev = chunk_get_prev(prev);

            if (chunk_is_token(prev, CT_DECLSPEC))
            {
               prev = chunk_get_prev(prev);
            }
         }

         // if it was determined that this could be a function definition
         // but one of the preceding tokens is a CT_MEMBER than this is not a
         // fcn def, issue #1466
         if (  isa_def
            && chunk_is_token(prev, CT_MEMBER))
         {
            isa_def = false;
         }

         // get first chunk before: A::B::pc | this.B.pc | this->B->pc
         if (  chunk_is_token(prev, CT_DC_MEMBER)
            || chunk_is_token(prev, CT_MEMBER))
         {
            while (  chunk_is_token(prev, CT_DC_MEMBER)
                  || chunk_is_token(prev, CT_MEMBER))
            {
               prev = chunk_get_prev_ncnlnp(prev);

               if (  prev == nullptr
                  || (  prev->type != CT_WORD
                     && prev->type != CT_TYPE
                     && prev->type != CT_THIS))
               {
                  LOG_FMT(LFCN, "%s(%d): --? skipped MEMBER and landed on %s\n",
                          __func__, __LINE__, (prev == nullptr) ? "<null>" : get_token_name(prev->type));
                  break;
               }
               LOG_FMT(LFCN, "%s(%d): <skip> '%s'\n",
                       __func__, __LINE__, prev->text());

               // Issue #1112
               // clarification: this will skip the CT_WORD, CT_TYPE or CT_THIS landing on either
               // another CT_DC_MEMBER or CT_MEMBER or a token that indicates the context of the
               // token in question; therefore, exit loop when not a CT_DC_MEMBER or CT_MEMBER
               prev = chunk_get_prev_ncnlnp(prev);

               if (prev == nullptr)
               {
                  LOG_FMT(LFCN, "%s(%d): prev is nullptr\n",
                          __func__, __LINE__);
               }
               else
               {
                  LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                          __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());
               }
            }

            if (prev == nullptr)
            {
               break;
            }
         }

         // If we are on a TYPE or WORD, then this could be a proto or def
         if (  chunk_is_token(prev, CT_TYPE)
            || chunk_is_token(prev, CT_WORD))
         {
            if (!hit_star)
            {
               LOG_FMT(LFCN, "%s(%d):   --> For sure a prototype or definition\n",
                       __func__, __LINE__);
               isa_def = true;
               break;
            }
            chunk_t *prev_prev = chunk_get_prev_ncnlnp(prev);

            if (!chunk_is_token(prev_prev, CT_QUESTION))               // Issue #1753
            {
               LOG_FMT(LFCN, "%s(%d):   --> maybe a proto/def\n",
                       __func__, __LINE__);

               LOG_FMT(LFCN, "%s(%d): prev is '%s', orig_line is %zu, orig_col is %zu, type is %s, parent_type is %s\n",
                       __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                       get_token_name(prev->type), get_token_name(get_chunk_parent_type(prev)));
               log_pcf_flags(LFCN, pc->flags);
               isa_def = true;
            }
         }

         if (chunk_is_ptr_operator(prev))
         {
            hit_star = true;
         }

         if (  prev->type != CT_OPERATOR
            && prev->type != CT_TSQUARE
            && prev->type != CT_ANGLE_CLOSE
            && prev->type != CT_QUALIFIER
            && prev->type != CT_TYPE
            && prev->type != CT_WORD
            && !chunk_is_ptr_operator(prev))
         {
            LOG_FMT(LFCN, "%s(%d):  --> Stopping on prev is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                    __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));

            // certain tokens are unlikely to precede a prototype or definition
            if (  chunk_is_token(prev, CT_ARITH)
               || chunk_is_token(prev, CT_ASSIGN)
               || chunk_is_token(prev, CT_COMMA)
               || (chunk_is_token(prev, CT_STRING) && get_chunk_parent_type(prev) != CT_EXTERN)  // fixes issue 1259
               || chunk_is_token(prev, CT_STRING_MULTI)
               || chunk_is_token(prev, CT_NUMBER)
               || chunk_is_token(prev, CT_NUMBER_FP)
               || chunk_is_token(prev, CT_FPAREN_OPEN)) // issue #1464
            {
               isa_def = false;
            }
            break;
         }

         // Skip over template and attribute stuff
         if (chunk_is_token(prev, CT_ANGLE_CLOSE))
         {
            prev = skip_template_prev(prev);
         }
         else
         {
            prev = chunk_get_prev_ncnlnp(prev);
         }
      }
      //LOG_FMT(LFCN, " -- stopped on %s [%s]\n",
      //        prev->text(), get_token_name(prev->type));

      // Fixes issue #1634
      if (chunk_is_paren_close(prev))
      {
         chunk_t *preproc = chunk_get_next_ncnl(prev);

         if (chunk_is_token(preproc, CT_PREPROC))
         {
            size_t pp_level = preproc->pp_level;

            if (chunk_is_token(chunk_get_next_ncnl(preproc), CT_PP_ELSE))
            {
               do
               {
                  preproc = chunk_get_prev_ncnlni(preproc);      // Issue #2279

                  if (chunk_is_token(preproc, CT_PP_IF))
                  {
                     preproc = chunk_get_prev_ncnlni(preproc);   // Issue #2279

                     if (preproc->pp_level == pp_level)
                     {
                        prev = chunk_get_prev_ncnlnp(preproc);
                        break;
                     }
                  }
               } while (preproc != nullptr);
            }
         }
      }

      if (  isa_def
         && prev != nullptr
         && (  (  chunk_is_paren_close(prev)
               && get_chunk_parent_type(prev) != CT_D_CAST
               && get_chunk_parent_type(prev) != CT_MACRO_OPEN  // Issue #2726
               && get_chunk_parent_type(prev) != CT_MACRO_CLOSE)
            || prev->type == CT_ASSIGN
            || prev->type == CT_RETURN))
      {
         LOG_FMT(LFCN, "%s(%d): -- overriding DEF due to prev is '%s', type is %s\n",
                 __func__, __LINE__, prev->text(), get_token_name(prev->type));
         isa_def = false;
      }

      // Fixes issue #1266, identification of a tuple return type in CS.
      if (  !isa_def
         && chunk_is_token(prev, CT_PAREN_CLOSE)
         && chunk_get_next_ncnl(prev) == pc)
      {
         tmp = chunk_skip_to_match_rev(prev);

         while (  tmp != nullptr                     // Issue #2315
               && tmp != prev)
         {
            if (chunk_is_token(tmp, CT_COMMA) && tmp->level == prev->level + 1)
            {
               LOG_FMT(LFCN, "%s(%d): -- overriding call due to tuple return type -- prev is '%s', type is %s\n",
                       __func__, __LINE__, prev->text(), get_token_name(prev->type));
               isa_def = true;
               break;
            }
            tmp = chunk_get_next_ncnl(tmp);
         }
      }

      if (isa_def)
      {
         LOG_FMT(LFCN, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
         LOG_FMT(LFCN, "%s(%d): (12) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         set_chunk_type(pc, CT_FUNC_DEF);

         if (prev == nullptr)
         {
            prev = chunk_get_head();
         }

         for (  tmp = prev; (tmp != nullptr)
             && tmp != pc; tmp = chunk_get_next_ncnlnp(tmp))
         {
            LOG_FMT(LFCN, "%s(%d): text() is '%s', type is %s\n",
                    __func__, __LINE__, tmp->text(), get_token_name(tmp->type));
            make_type(tmp);
         }
      }
   }

   if (pc->type != CT_FUNC_DEF)
   {
      LOG_FMT(LFCN, "%s(%d):  Detected type %s, text() is '%s', on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, get_token_name(pc->type),
              pc->text(), pc->orig_line, pc->orig_col);

      tmp = flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);

      if (  chunk_is_token(tmp, CT_BRACE_OPEN)
         && get_chunk_parent_type(tmp) != CT_DOUBLE_BRACE)
      {
         set_paren_parent(tmp, pc->type);
      }
      return;
   }
   /*
    * We have a function definition or prototype
    * Look for a semicolon or a brace open after the close parenthesis to figure
    * out whether this is a prototype or definition
    */

   // See if this is a prototype or implementation

   // FIXME: this doesn't take the old K&R parameter definitions into account

   // Scan tokens until we hit a brace open (def) or semicolon (proto)
   tmp = paren_close;

   while ((tmp = chunk_get_next_ncnl(tmp)) != nullptr)
   {
      // Only care about brace or semicolon on the same level
      if (tmp->level < pc->level)
      {
         // No semicolon - guess that it is a prototype
         chunk_flags_clr(pc, PCF_VAR_1ST_DEF);
         set_chunk_type(pc, CT_FUNC_PROTO);
         break;
      }
      else if (tmp->level == pc->level)
      {
         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            // its a function def for sure
            break;
         }
         else if (chunk_is_semicolon(tmp))
         {
            // Set the parent for the semicolon for later
            semi = tmp;
            chunk_flags_clr(pc, PCF_VAR_1ST_DEF);
            set_chunk_type(pc, CT_FUNC_PROTO);
            LOG_FMT(LFCN, "%s(%d):   2) Marked text() is '%s', as FUNC_PROTO on orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            break;
         }
         else if (chunk_is_token(pc, CT_COMMA))
         {
            set_chunk_type(pc, CT_FUNC_CTOR_VAR);
            LOG_FMT(LFCN, "%s(%d):   2) Marked text() is '%s', as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            break;
         }
      }
   }

   /*
    * C++ syntax is wacky. We need to check to see if a prototype is really a
    * variable definition with parameters passed into the constructor.
    * Unfortunately, without being able to accurately determine if an
    * identifier is a type (which would require us to more or less be a full
    * compiler), the only mostly reliable way to do so is to guess that it is
    * a constructor variable if inside a function body and scan the 'parameter
    * list' for items that are not allowed in a prototype. We search backwards
    * and checking the parent of the containing open braces. If the parent is a
    * class or namespace, then it probably is a prototype.
    */
   if (  language_is_set(LANG_CPP)
      && chunk_is_token(pc, CT_FUNC_PROTO)
      && get_chunk_parent_type(pc) != CT_OPERATOR)
   {
      LOG_FMT(LFPARAM, "%s(%d):", __func__, __LINE__);
      LOG_FMT(LFPARAM, "  checking '%s' for constructor variable %s %s\n",
              pc->text(),
              get_token_name(paren_open->type),
              get_token_name(paren_close->type));

      /*
       * Check the token at the start of the statement. If it's 'extern', we
       * definitely have a function prototype.
       */
      tmp = pc;

      while (  tmp != nullptr
            && !tmp->flags.test(PCF_STMT_START))
      {
         tmp = chunk_get_prev_ncnlni(tmp);   // Issue #2279
      }
      const bool is_extern = (tmp && tmp->str.equals("extern"));

      /*
       * Scan the parameters looking for:
       *  - constant strings
       *  - numbers
       *  - non-type fields
       *  - function calls
       */
      chunk_t *ref = chunk_get_next_ncnl(paren_open);
      chunk_t *tmp2;
      bool    is_param = true;
      tmp = ref;

      while (tmp != paren_close)
      {
         tmp2 = chunk_get_next_ncnl(tmp);

         if (  chunk_is_token(tmp, CT_COMMA)
            && (tmp->level == (paren_open->level + 1)))
         {
            if (!can_be_full_param(ref, tmp))
            {
               is_param = false;
               break;
            }
            ref = tmp2;
         }
         tmp = tmp2;
      }

      if (  !is_extern
         && is_param && ref != tmp)
      {
         if (!can_be_full_param(ref, tmp))
         {
            is_param = false;
         }
      }

      if (  !is_extern
         && !is_param)
      {
         set_chunk_type(pc, CT_FUNC_CTOR_VAR);
         LOG_FMT(LFCN, "%s(%d):   3) Marked text() '%s' as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      }
      else if (pc->brace_level > 0)
      {
         chunk_t *br_open = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->brace_level - 1);

         if (  br_open != nullptr
            && get_chunk_parent_type(br_open) != CT_EXTERN
            && get_chunk_parent_type(br_open) != CT_NAMESPACE)
         {
            // Do a check to see if the level is right
            prev = chunk_get_prev_ncnlni(pc);   // Issue #2279

            if (  !chunk_is_str(prev, "*", 1)
               && !chunk_is_str(prev, "&", 1))
            {
               chunk_t *p_op = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->brace_level - 1);

               if (  p_op != nullptr
                  && get_chunk_parent_type(p_op) != CT_CLASS
                  && get_chunk_parent_type(p_op) != CT_STRUCT
                  && get_chunk_parent_type(p_op) != CT_NAMESPACE)
               {
                  set_chunk_type(pc, CT_FUNC_CTOR_VAR);
                  LOG_FMT(LFCN, "%s(%d):   4) Marked text() is'%s', as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                          __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               }
            }
         }
      }
   }

   if (semi != nullptr)
   {
      set_chunk_parent(semi, pc->type);
   }

   // Issue # 1403, 2152
   if (chunk_is_token(paren_open->prev, CT_FUNC_CTOR_VAR))
   {
      flag_parens(paren_open, PCF_IN_FCN_CTOR, CT_FPAREN_OPEN, pc->type, false);
   }
   else
   {
      flag_parens(paren_open, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, pc->type, false);
   }
   //flag_parens(paren_open, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, pc->type, true);

   if (chunk_is_token(pc, CT_FUNC_CTOR_VAR))
   {
      chunk_flags_set(pc, PCF_VAR_1ST_DEF);
      return;
   }

   if (chunk_is_token(next, CT_TSQUARE))
   {
      next = chunk_get_next_ncnl(next);

      if (next == nullptr)
      {
         return;
      }
   }
   // Mark parameters and return type
   fix_fcn_def_params(next);
   mark_function_return_type(pc, chunk_get_prev_ncnlni(pc), pc->type);   // Issue #2279

   /* mark C# where chunk */
   if (  language_is_set(LANG_CS)
      && (  (chunk_is_token(pc, CT_FUNC_DEF))
         || (chunk_is_token(pc, CT_FUNC_PROTO))))
   {
      tmp = chunk_get_next_ncnl(paren_close);
      pcf_flags_t in_where_spec_flags = PCF_NONE;

      while (  (tmp != nullptr)
            && (tmp->type != CT_BRACE_OPEN)
            && (tmp->type != CT_SEMICOLON))
      {
         mark_where_chunk(tmp, pc->type, tmp->flags | in_where_spec_flags);
         in_where_spec_flags = tmp->flags & PCF_IN_WHERE_SPEC;

         tmp = chunk_get_next_ncnl(tmp);
      }
   }

   // Find the brace pair and set the parent
   if (chunk_is_token(pc, CT_FUNC_DEF))
   {
      tmp = chunk_get_next_ncnl(paren_close);

      while (  tmp != nullptr
            && tmp->type != CT_BRACE_OPEN)
      {
         LOG_FMT(LFCN, "%s(%d): (13) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());
         set_chunk_parent(tmp, CT_FUNC_DEF);

         if (!chunk_is_semicolon(tmp))
         {
            chunk_flags_set(tmp, PCF_OLD_FCN_PARAMS);
         }
         tmp = chunk_get_next_ncnl(tmp);
      }

      if (chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): (14) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());
         set_chunk_parent(tmp, CT_FUNC_DEF);
         tmp = chunk_skip_to_match(tmp);

         if (tmp != nullptr)
         {
            LOG_FMT(LFCN, "%s(%d): (15) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                    __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());
            set_chunk_parent(tmp, CT_FUNC_DEF);
         }
      }
   }
} // mark_function


void mark_cpp_constructor(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *paren_open;
   chunk_t *tmp;
   chunk_t *after;
   chunk_t *var;
   bool    is_destr = false;

   tmp = chunk_get_prev_ncnlni(pc);   // Issue #2279

   if (chunk_is_token(tmp, CT_INV) || chunk_is_token(tmp, CT_DESTRUCTOR))
   {
      set_chunk_type(tmp, CT_DESTRUCTOR);
      set_chunk_parent(pc, CT_DESTRUCTOR);
      is_destr = true;
   }
   LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, FOUND %sSTRUCTOR for '%s'[%s] prev '%s'[%s]\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col,
           is_destr ? "DE" : "CON",
           pc->text(), get_token_name(pc->type),
           tmp->text(), get_token_name(tmp->type));

   paren_open = skip_template_next(chunk_get_next_ncnl(pc));

   if (!chunk_is_str(paren_open, "(", 1))
   {
      LOG_FMT(LWARN, "%s:%zu Expected '(', got: [%s]\n",
              cpd.filename.c_str(), paren_open->orig_line,
              paren_open->text());
      return;
   }
   // Mark parameters
   fix_fcn_def_params(paren_open);
   after = flag_parens(paren_open, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CLASS_PROTO, false);

   LOG_FMT(LFTOR, "%s(%d): text() '%s'\n", __func__, __LINE__, after->text());

   // Scan until the brace open, mark everything
   tmp = paren_open;
   bool hit_colon = false;

   while (  tmp != nullptr
         && (tmp->type != CT_BRACE_OPEN || tmp->level != paren_open->level)
         && !chunk_is_semicolon(tmp))
   {
      LOG_FMT(LFTOR, "%s(%d): tmp is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col);
      chunk_flags_set(tmp, PCF_IN_CONST_ARGS);
      tmp = chunk_get_next_ncnl(tmp);

      if (chunk_is_str(tmp, ":", 1) && tmp->level == paren_open->level)
      {
         set_chunk_type(tmp, CT_CONSTR_COLON);
         hit_colon = true;
      }

      if (  hit_colon
         && (chunk_is_paren_open(tmp) || chunk_is_opening_brace(tmp))
         && tmp->level == paren_open->level)
      {
         var = skip_template_prev(chunk_get_prev_ncnlni(tmp));   // Issue #2279

         if (chunk_is_token(var, CT_TYPE) || chunk_is_token(var, CT_WORD))
         {
            set_chunk_type(var, CT_FUNC_CTOR_VAR);
            flag_parens(tmp, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CTOR_VAR, false);
         }
      }
   }

   if (chunk_is_token(tmp, CT_BRACE_OPEN))
   {
      set_paren_parent(paren_open, CT_FUNC_CLASS_DEF);
      set_paren_parent(tmp, CT_FUNC_CLASS_DEF);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_DEF on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   }
   else
   {
      set_chunk_parent(tmp, CT_FUNC_CLASS_PROTO);
      set_chunk_type(pc, CT_FUNC_CLASS_PROTO);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_PROTO on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   }
} // mark_cpp_constructor


pcf_flags_t mark_where_chunk(chunk_t *pc, c_token_t parent_type, pcf_flags_t flags)
{
   /* TODO: should have options to control spacing around the ':' as well as newline ability for the
    * constraint clauses (should it break up a 'where A : B where C : D' on the same line? wrap? etc.) */

   if (chunk_is_token(pc, CT_WHERE))
   {
      set_chunk_type(pc, CT_WHERE_SPEC);
      set_chunk_parent(pc, parent_type);
      flags |= PCF_IN_WHERE_SPEC;
      LOG_FMT(LFTOR, "%s: where-spec on line %zu\n",
              __func__, pc->orig_line);
   }
   else if (flags.test(PCF_IN_WHERE_SPEC))
   {
      if (chunk_is_str(pc, ":", 1))
      {
         set_chunk_type(pc, CT_WHERE_COLON);
         LOG_FMT(LFTOR, "%s: where-spec colon on line %zu\n",
                 __func__, pc->orig_line);
      }
      else if ((chunk_is_token(pc, CT_STRUCT)) || (chunk_is_token(pc, CT_CLASS)))
      {
         /* class/struct inside of a where-clause confuses parser for indentation; set it as a word so it looks like the rest */
         set_chunk_type(pc, CT_WORD);
      }
   }

   if (flags.test(PCF_IN_WHERE_SPEC))
   {
      chunk_flags_set(pc, PCF_IN_WHERE_SPEC);
   }
   return(flags);
}


void mark_class_ctor(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, start is '%s', parent_type is %s\n",
           __func__, __LINE__, start->orig_line, start->orig_col, start->text(),
           get_token_name(get_chunk_parent_type(start)));
   log_pcf_flags(LFTOR, start->flags);

   chunk_t *pclass = chunk_get_next_ncnl(start, scope_e::PREPROC);

   LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
           __func__, __LINE__, pclass->text());
   log_pcf_flags(LFTOR, pclass->flags);

   if (language_is_set(LANG_CPP))
   {
      pclass = skip_attribute_next(pclass);
      LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
              __func__, __LINE__, pclass->text());
   }

   if (get_chunk_parent_type(start) == CT_TEMPLATE)
   {
      // look after the class name
      chunk_t *openingTemplate = chunk_get_next_ncnl(pclass);
      LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, openingTemplate is '%s', type is %s\n",
              __func__, __LINE__, openingTemplate->orig_line, openingTemplate->orig_col,
              openingTemplate->text(), get_token_name(openingTemplate->type));

      if (chunk_is_token(openingTemplate, CT_ANGLE_OPEN))
      {
         chunk_t *closingTemplate = chunk_skip_to_match(openingTemplate);
         LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, closingTemplate is '%s', type is %s\n",
                 __func__, __LINE__, closingTemplate->orig_line, closingTemplate->orig_col,
                 closingTemplate->text(), get_token_name(closingTemplate->type));
         chunk_t *thirdToken = chunk_get_next_ncnl(closingTemplate);
         LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, thirdToken is '%s', type is %s\n",
                 __func__, __LINE__, thirdToken->orig_line, thirdToken->orig_col,
                 thirdToken->text(), get_token_name(thirdToken->type));

         if (chunk_is_token(thirdToken, CT_DC_MEMBER))
         {
            pclass = chunk_get_next_ncnl(thirdToken);
            LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, pclass is '%s', type is %s\n",
                    __func__, __LINE__, pclass->orig_line, pclass->orig_col,
                    pclass->text(), get_token_name(pclass->type));
         }
      }
   }
   pclass = skip_attribute_next(pclass);
   LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
           __func__, __LINE__, pclass->text());

   if (chunk_is_token(pclass, CT_DECLSPEC))  // Issue 1289
   {
      pclass = chunk_get_next_ncnl(pclass);
      LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
              __func__, __LINE__, pclass->text());

      if (chunk_is_token(pclass, CT_PAREN_OPEN))
      {
         pclass = chunk_get_next_ncnl(chunk_skip_to_match(pclass));
         LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
                 __func__, __LINE__, pclass->text());
      }
   }

   if (  pclass == nullptr
      || (pclass->type != CT_TYPE && pclass->type != CT_WORD))
   {
      return;
   }
   chunk_t *next = chunk_get_next_ncnl(pclass, scope_e::PREPROC);

   while (  chunk_is_token(next, CT_TYPE)
         || chunk_is_token(next, CT_WORD)
         || chunk_is_token(next, CT_DC_MEMBER))
   {
      pclass = next;
      LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
              __func__, __LINE__, pclass->text());
      next = chunk_get_next_ncnl(next, scope_e::PREPROC);
   }
   chunk_t *pc   = chunk_get_next_ncnl(pclass, scope_e::PREPROC);
   size_t  level = pclass->brace_level + 1;

   if (pc == nullptr)
   {
      LOG_FMT(LFTOR, "%s(%d): Called on %s on orig_line %zu. Bailed on NULL\n",
              __func__, __LINE__, pclass->text(), pclass->orig_line);
      return;
   }
   // Add the class name
   ChunkStack cs;

   cs.Push_Back(pclass);

   LOG_FMT(LFTOR, "%s(%d): Called on %s on orig_line %zu (next is '%s')\n",
           __func__, __LINE__, pclass->text(), pclass->orig_line, pc->text());

   // detect D template class: "class foo(x) { ... }"
   if (language_is_set(LANG_D) && chunk_is_token(next, CT_PAREN_OPEN))              // Coverity CID 76004
   {
      set_chunk_parent(next, CT_TEMPLATE);

      next = get_d_template_types(cs, next);

      if (chunk_is_token(next, CT_PAREN_CLOSE))
      {
         set_chunk_parent(next, CT_TEMPLATE);
      }
   }
   // Find the open brace, abort on semicolon
   pcf_flags_t flags = PCF_NONE;

   while (pc != nullptr && pc->type != CT_BRACE_OPEN)
   {
      LOG_FMT(LFTOR, " [%s]", pc->text());

      flags = mark_where_chunk(pc, start->type, flags);

      if (!flags.test(PCF_IN_WHERE_SPEC) && chunk_is_str(pc, ":", 1))
      {
         set_chunk_type(pc, CT_CLASS_COLON);
         flags |= PCF_IN_CLASS_BASE;
         LOG_FMT(LFTOR, "%s(%d): class colon on line %zu\n",
                 __func__, __LINE__, pc->orig_line);
      }

      if (chunk_is_semicolon(pc))
      {
         LOG_FMT(LFTOR, "%s(%d): bailed on semicolon on line %zu\n",
                 __func__, __LINE__, pc->orig_line);
         return;
      }
      chunk_flags_set(pc, flags);
      pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
   }

   if (pc == nullptr)
   {
      LOG_FMT(LFTOR, "%s(%d): bailed on NULL\n", __func__, __LINE__);
      return;
   }
   set_paren_parent(pc, start->type);
   chunk_flags_set(pc, PCF_IN_CLASS);

   pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
   LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
           __func__, __LINE__, pclass->text());

   while (pc != nullptr)
   {
      LOG_FMT(LFTOR, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      chunk_flags_set(pc, PCF_IN_CLASS);

      if (  pc->brace_level > level
         || pc->level > pc->brace_level
         || pc->flags.test(PCF_IN_PREPROC))
      {
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if (chunk_is_token(pc, CT_BRACE_CLOSE) && pc->brace_level < level)
      {
         LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, Hit brace close\n",
                 __func__, __LINE__, pc->orig_line);
         pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);

         if (chunk_is_token(pc, CT_SEMICOLON))
         {
            set_chunk_parent(pc, start->type);
         }
         return;
      }
      next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

      if (chunkstack_match(cs, pc))
      {
         LOG_FMT(LFTOR, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
         // Issue #1333 Formatter removes semicolon after variable initializer at class level(C#)
         // if previous chunk is 'new' operator it is variable initializer not a CLASS_FUNC_DEF.
         chunk_t *prev = chunk_get_prev_ncnlni(pc, scope_e::PREPROC);   // Issue #2279
         LOG_FMT(LFTOR, "%s(%d): prev is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));

         // Issue #1003, next->type should not be CT_FPAREN_OPEN
         if (  prev != nullptr
            && (prev->type != CT_NEW))
         {
            bool is_func_class_def = false;

            if (chunk_is_token(next, CT_PAREN_OPEN))
            {
               is_func_class_def = true;
            }
            else if (chunk_is_token(next, CT_ANGLE_OPEN))          // Issue # 1737
            {
               chunk_t *closeAngle    = chunk_skip_to_match(next);
               chunk_t *afterTemplate = chunk_get_next(closeAngle);

               if (chunk_is_token(afterTemplate, CT_PAREN_OPEN))
               {
                  is_func_class_def = true;
               }
            }
            else
            {
               LOG_FMT(LFTOR, "%s(%d): text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
               make_type(pc);
            }

            if (is_func_class_def)
            {
               set_chunk_type(pc, CT_FUNC_CLASS_DEF);
               LOG_FMT(LFTOR, "%s(%d): text() is '%s', orig_line is %zu, orig_col is %zu, type is %s, Marked CTor/DTor\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
               mark_cpp_constructor(pc);
            }
         }
      }
      pc = next;
   }
} // mark_class_ctor


void mark_struct_union_body(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   while (  pc != nullptr
         && pc->level >= start->level
         && !(pc->level == start->level && chunk_is_token(pc, CT_BRACE_CLOSE)))
   {
      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_SEMICOLON))
      {
         pc = chunk_get_next_ncnl(pc);

         if (pc == nullptr)
         {
            break;
         }
      }

      if (chunk_is_token(pc, CT_ALIGN))
      {
         pc = skip_align(pc); // "align(x)" or "align(x):"

         if (pc == nullptr)
         {
            break;
         }
      }
      else
      {
         pc = fix_variable_definition(pc);

         if (pc == nullptr)
         {
            break;
         }
      }
   }
} // mark_struct_union_body


void mark_comments(void)
{
   LOG_FUNC_ENTRY();

   cpd.unc_stage = unc_stage_e::MARK_COMMENTS;

   bool    prev_nl = true;
   chunk_t *cur    = chunk_get_head();

   while (cur != nullptr)
   {
      chunk_t *next   = chunk_get_next_nvb(cur);
      bool    next_nl = (next == nullptr) || chunk_is_newline(next);

      if (chunk_is_comment(cur))
      {
         if (next_nl && prev_nl)
         {
            set_chunk_parent(cur, CT_COMMENT_WHOLE);
         }
         else if (next_nl)
         {
            set_chunk_parent(cur, CT_COMMENT_END);
         }
         else if (prev_nl)
         {
            set_chunk_parent(cur, CT_COMMENT_START);
         }
         else
         {
            set_chunk_parent(cur, CT_COMMENT_EMBED);
         }
      }
      prev_nl = chunk_is_newline(cur);
      cur     = next;
   }
}


void mark_define_expressions(void)
{
   LOG_FUNC_ENTRY();

   bool    in_define = false;
   bool    first     = true;
   chunk_t *pc       = chunk_get_head();
   chunk_t *prev     = pc;

   while (pc != nullptr)
   {
      if (!in_define)
      {
         if (  chunk_is_token(pc, CT_PP_DEFINE)
            || chunk_is_token(pc, CT_PP_IF)
            || chunk_is_token(pc, CT_PP_ELSE))
         {
            in_define = true;
            first     = true;
         }
      }
      else
      {
         if (!pc->flags.test(PCF_IN_PREPROC) || chunk_is_token(pc, CT_PREPROC))
         {
            in_define = false;
         }
         else
         {
            if (  pc->type != CT_MACRO
               && (  first
                  || chunk_is_token(prev, CT_PAREN_OPEN)
                  || chunk_is_token(prev, CT_ARITH)
                  || chunk_is_token(prev, CT_CARET)
                  || chunk_is_token(prev, CT_ASSIGN)
                  || chunk_is_token(prev, CT_COMPARE)
                  || chunk_is_token(prev, CT_RETURN)
                  || chunk_is_token(prev, CT_GOTO)
                  || chunk_is_token(prev, CT_CONTINUE)
                  || chunk_is_token(prev, CT_FPAREN_OPEN)
                  || chunk_is_token(prev, CT_SPAREN_OPEN)
                  || chunk_is_token(prev, CT_BRACE_OPEN)
                  || chunk_is_semicolon(prev)
                  || chunk_is_token(prev, CT_COMMA)
                  || chunk_is_token(prev, CT_COLON)
                  || chunk_is_token(prev, CT_QUESTION)))
            {
               chunk_flags_set(pc, PCF_EXPR_START);
               first = false;
            }
         }
      }
      prev = pc;
      pc   = chunk_get_next(pc);
   }
} // mark_define_expressions


void mark_template_func(chunk_t *pc, chunk_t *pc_next)
{
   LOG_FUNC_ENTRY();

   // We know angle_close must be there...
   chunk_t *angle_close = chunk_get_next_type(pc_next, CT_ANGLE_CLOSE, pc->level);
   chunk_t *after       = chunk_get_next_ncnl(angle_close);

   if (after != nullptr)
   {
      if (chunk_is_str(after, "(", 1))
      {
         if (angle_close->flags.test(PCF_IN_FCN_CALL))
         {
            LOG_FMT(LTEMPFUNC, "%s(%d): marking '%s' in line %zu as a FUNC_CALL\n",
                    __func__, __LINE__, pc->text(), pc->orig_line);
            LOG_FMT(LFCN, "%s(%d): (16) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            set_chunk_type(pc, CT_FUNC_CALL);
            flag_parens(after, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
         }
         else
         {
            /*
             * Might be a function def. Must check what is before the template:
             * Func call:
             *   BTree.Insert(std::pair<int, double>(*it, double(*it) + 1.0));
             *   a = Test<int>(j);
             *   std::pair<int, double>(*it, double(*it) + 1.0));
             */

            LOG_FMT(LTEMPFUNC, "%s(%d): marking '%s' in line %zu as a FUNC_CALL 2\n",
                    __func__, __LINE__, pc->text(), pc->orig_line);
            // its a function!!!
            LOG_FMT(LFCN, "%s(%d): (17) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            set_chunk_type(pc, CT_FUNC_CALL);
            mark_function(pc);
         }
      }
      else if (chunk_is_token(after, CT_WORD))
      {
         // its a type!
         set_chunk_type(pc, CT_TYPE);
         chunk_flags_set(pc, PCF_VAR_TYPE);
         chunk_flags_set(after, PCF_VAR_DEF);
      }
   }
} // mark_template_func


void mark_exec_sql(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   // Change CT_WORD to CT_SQL_WORD
   for (tmp = chunk_get_next(pc); tmp != nullptr; tmp = chunk_get_next(tmp))
   {
      set_chunk_parent(tmp, pc->type);

      if (chunk_is_token(tmp, CT_WORD))
      {
         set_chunk_type(tmp, CT_SQL_WORD);
      }

      if (chunk_is_token(tmp, CT_SEMICOLON))
      {
         break;
      }
   }

   if (  pc->type != CT_SQL_BEGIN
      || tmp == nullptr
      || tmp->type != CT_SEMICOLON)
   {
      return;
   }

   for (tmp = chunk_get_next(tmp);
        tmp != nullptr && tmp->type != CT_SQL_END;
        tmp = chunk_get_next(tmp))
   {
      tmp->level++;
   }
}
