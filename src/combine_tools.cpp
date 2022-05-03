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


bool can_be_full_param(Chunk *start, Chunk *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFPARAM, "%s(%d): start->Text() is '%s', type is %s\n",
           __func__, __LINE__, start->Text(), get_token_name(start->type));
   LOG_FMT(LFPARAM, "%s(%d): end->Text()   is '%s', type is %s\n",
           __func__, __LINE__, end->Text(), get_token_name(end->type));

   int   word_count     = 0;
   int   type_count     = 0;
   Chunk *pc            = nullptr;
   Chunk *first_word    = nullptr;
   bool  first_word_set = false;

   for (pc = start;
        pc != nullptr && pc->IsNotNullChunk() && pc != end;
        pc = pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->type));

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
         LOG_FMT(LFPARAM, "%s(%d): <== elipses\n",
                 __func__, __LINE__);

         return(true);
      }
      else if (  word_count == 0
              && chunk_is_token(pc, CT_PAREN_OPEN))
      {
         // Check for old-school func proto param '(type)'
         Chunk *tmp1 = chunk_skip_to_match(pc, E_Scope::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }
         Chunk *tmp2 = tmp1->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp2->IsNullChunk())
         {
            return(false);
         }

         if (  chunk_is_token(tmp2, CT_COMMA)
            || chunk_is_paren_close(tmp2))
         {
            do
            {
               pc = pc->GetNextNcNnl(E_Scope::PREPROC);

               if (pc->IsNullChunk())
               {
                  return(false);
               }
               LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', type is %s\n",
                       __func__, __LINE__, pc->Text(), get_token_name(pc->type));
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
         Chunk *tmp1 = pc->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp1->IsNullChunk())
         {
            return(false);
         }
         Chunk *tmp2 = tmp1->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp2->IsNullChunk())
         {
            return(false);
         }
         Chunk *tmp3 = (chunk_is_str(tmp2, ")")) ? tmp2 : tmp2->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp3->IsNullChunk())
         {
            return(false);
         }

         if (  !chunk_is_str(tmp3, ")")
            || !(  chunk_is_str(tmp1, "*")
                || chunk_is_str(tmp1, "^")) // Issue #2656
            || !(  tmp2->type == CT_WORD
                || chunk_is_str(tmp2, ")")))
         {
            LOG_FMT(LFPARAM, "%s(%d): <== '%s' not fcn type!\n",
                    __func__, __LINE__, get_token_name(pc->type));
            return(false);
         }
         LOG_FMT(LFPARAM, "%s(%d): <skip fcn type>\n",
                 __func__, __LINE__);

         tmp1 = tmp3->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp1->IsNullChunk())
         {
            return(false);
         }

         if (chunk_is_str(tmp1, "("))
         {
            tmp3 = chunk_skip_to_match(tmp1, E_Scope::PREPROC);
         }
         pc = tmp3;
         LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(pc->type));

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
         pc = chunk_skip_to_match(pc, E_Scope::PREPROC);
         LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(pc->type));
      }
      else if (  word_count == 2
              && chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         // Bug #671: is it such as: bool foo[FOO_MAX]
         pc = chunk_skip_to_match(pc, E_Scope::PREPROC);
         LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', type is %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(pc->type));
      }
      else if (  word_count == 1
              && language_is_set(LANG_CPP)
              && chunk_is_str(pc, "&&"))
      {
         // ignore possible 'move' operator
      }
      else
      {
         LOG_FMT(LFPARAM, "%s(%d): <== type is %s, no way!, type count is %d, word count is %d\n",
                 __func__, __LINE__, get_token_name(pc->type), type_count, word_count);
         return(false);
      }
      LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->type));
   }

   Chunk *last = pc->GetPrevNcNnlNi();   // Issue #2279

   LOG_FMT(LFPARAM, "%s(%d): last->Text() is '%s', type is %s\n",
           __func__, __LINE__, last->Text(), get_token_name(last->type));

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
      Chunk *brace =
         start->GetPrevType(CT_BRACE_OPEN, start->brace_level - 1);

      if (brace->IsNotNullChunk())
      {
         LOG_FMT(LFPARAM, "%s(%d): (matching %s brace at orig_line %zu, orig_col is %zu)",
                 __func__, __LINE__,
                 get_token_name(get_chunk_parent_type(brace)), brace->orig_line, brace->orig_col);
      }

      if (  brace->IsNotNullChunk()
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
   LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', word_count is %d, type_count is %d\n",
           __func__, __LINE__, pc->Text(), word_count, type_count);

   if (first_word != nullptr)
   {
      LOG_FMT(LFPARAM, "%s(%d): first_word->Text() is '%s'\n",
              __func__, __LINE__, first_word->Text());
   }
   bool ret = (  word_count >= 2
              || (  word_count == 1
                 && type_count == 1));

   LOG_FMT(LFPARAM, "%s(%d): ret is %s\n",
           __func__, __LINE__, ret ? "TRUE" : "FALSE");

   LOG_FMT(LFPARAM, "%s(%d): pc->Text() is '%s', ",
           __func__, __LINE__, pc->Text());
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


bool chunk_ends_type(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk  *pc       = start;
   bool   ret       = false;
   size_t cnt       = 0;
   bool   last_expr = false;
   bool   last_lval = false;

   bool   a = pc->flags.test(PCF_IN_FCN_CTOR);

   if (a)
   {
      return(false);
   }

   for ( ; pc->IsNotNullChunk(); pc = pc->GetPrevNcNnlNi()) // Issue #2279
   {
      LOG_FMT(LFTYPE, "%s(%d): type is %s, Text() '%s', orig_line %zu, orig_col %zu\n   ",
              __func__, __LINE__, get_token_name(pc->type), pc->Text(),
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
         || (  language_is_set(LANG_CS | LANG_VALA)
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

   if (pc->IsNullChunk())
   {
      // first token
      ret = true;
   }
   LOG_FMT(LFTYPE, "%s(%d): first token verdict: %s\n",
           __func__, __LINE__, ret ? "yes" : "no");

   return(ret);
} // chunk_ends_type


bool chunkstack_match(ChunkStack &cs, Chunk *pc)
{
   for (size_t idx = 0; idx < cs.Len(); idx++)
   {
      Chunk *tmp = cs.GetChunk(idx);

      if (pc->str.equals(tmp->str))
      {
         return(true);
      }
   }

   return(false);
} // chunkstack_match


void flag_series(Chunk *start, Chunk *end, pcf_flags_t set_flags, pcf_flags_t clr_flags, E_Scope nav)
{
   LOG_FUNC_ENTRY();

   while (  start != nullptr
         && start->IsNotNullChunk()
         && start != end)
   {
      chunk_flags_upd(start, clr_flags, set_flags);

      start = start->GetNext(nav);

      if (start->IsNullChunk())
      {
         return;
      }
   }

   if (  end != nullptr
      && end->IsNotNullChunk())
   {
      chunk_flags_upd(end, clr_flags, set_flags);
   }
} // flag_series


size_t get_cpp_template_angle_nest_level(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   int nestLevel = 0;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   while (  pc->IsNotNullChunk()
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
      pc = pc->GetPrevNcNnlNi();
   }
   return(nestLevel <= 0 ? 0 : size_t(nestLevel));
}


Chunk *get_d_template_types(ChunkStack &cs, Chunk *open_paren)
{
   LOG_FUNC_ENTRY();
   Chunk *tmp       = open_paren->GetNextNcNnl();
   bool  maybe_type = true;

   while (  tmp->IsNullChunk()
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
      tmp = tmp->GetNextNcNnl();
   }
   return(tmp);
} // get_d_template_types


bool go_on(Chunk *pc, Chunk *start)
{
   if (  pc == nullptr
      || pc->IsNullChunk()
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


void make_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_WORD))
      {
         set_chunk_type(pc, CT_TYPE);
      }
      else if (  (  pc->IsStar()
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


Chunk *set_paren_parent(Chunk *start, E_Token parent)
{
   LOG_FUNC_ENTRY();
   Chunk *end;

   end = chunk_skip_to_match(start, E_Scope::PREPROC);

   if (end != nullptr)
   {
      LOG_FMT(LFLPAREN, "%s(%d): %zu:%zu '%s' and %zu:%zu '%s' type is %s, parent_type is %s",
              __func__, __LINE__, start->orig_line, start->orig_col, start->Text(),
              end->orig_line, end->orig_col, end->Text(),
              get_token_name(start->type), get_token_name(parent));
      log_func_stack_inline(LFLPAREN);
      set_chunk_parent(start, parent);
      set_chunk_parent(end, parent);
      LOG_FMT(LFLPAREN, "%s(%d):\n", __func__, __LINE__);
      return(end->GetNextNcNnl(E_Scope::PREPROC));
   }
   LOG_FMT(LFLPAREN, "%s(%d):\n", __func__, __LINE__);
   return(nullptr);
} // set_paren_parent
