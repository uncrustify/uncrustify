/**
 * @file combine_fix_mark.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract fom combine.cpp
 */

#include "combine_fix_mark.h"

#include "combine_skip.h"
#include "combine_tools.h"
#include "flag_parens.h"
#include "log_rules.h"

constexpr static auto LCURRENT = LCOMBINE;


void fix_casts(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk      *pc;
   Chunk      *prev;
   Chunk      *first;
   Chunk      *after;
   Chunk      *last = nullptr;
   Chunk      *paren_close;
   const char *verb      = "likely";
   const char *detail    = "";
   size_t     count      = 0;
   int        word_count = 0;
   bool       nope;
   bool       doubtful_cast = false;


   LOG_FMT(LCASTS, "%s(%d): start->Text() is '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->Text(), start->orig_line, start->orig_col);

   prev = start->GetPrevNcNnlNi();   // Issue #2279

   if (prev->IsNullChunk())
   {
      return;
   }

   if (chunk_is_token(prev, CT_PP_DEFINED))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - after defined\n",
              __func__, __LINE__);
      return;
   }

   if (chunk_is_token(prev, CT_ANGLE_CLOSE))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - after > (template)\n",
              __func__, __LINE__);
      return;
   }
   // Make sure there is only WORD, TYPE, and '*' or '^' before the close paren
   pc    = start->GetNextNcNnl();
   first = pc;

   while (  pc->IsNotNullChunk()
         && (  chunk_is_type(pc)
            || chunk_is_token(pc, CT_WORD)
            || chunk_is_token(pc, CT_QUALIFIER)
            || chunk_is_token(pc, CT_DC_MEMBER)
            || chunk_is_token(pc, CT_PP)
            || chunk_is_token(pc, CT_STAR)
            || chunk_is_token(pc, CT_QUESTION)
            || chunk_is_token(pc, CT_CARET)
            || chunk_is_token(pc, CT_TSQUARE)
            || (  (  chunk_is_token(pc, CT_ANGLE_OPEN)
                  || chunk_is_token(pc, CT_ANGLE_CLOSE))
               && language_is_set(LANG_OC | LANG_JAVA | LANG_CS | LANG_VALA | LANG_CPP))
            || (  (  chunk_is_token(pc, CT_QUESTION)
                  || chunk_is_token(pc, CT_COMMA)
                  || chunk_is_token(pc, CT_MEMBER))
               && language_is_set(LANG_JAVA | LANG_CS | LANG_VALA))
            || (  chunk_is_token(pc, CT_COMMA)
               && language_is_set(LANG_CPP))
            || chunk_is_token(pc, CT_AMP)))
   {
      LOG_FMT(LCASTS, "%s(%d): pc->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));

      if (  chunk_is_token(pc, CT_WORD)
         || (  chunk_is_token(last, CT_ANGLE_CLOSE)
            && chunk_is_token(pc, CT_DC_MEMBER)))
      {
         word_count++;
      }
      else if (  chunk_is_token(pc, CT_DC_MEMBER)
              || chunk_is_token(pc, CT_MEMBER)
              || chunk_is_token(pc, CT_PP))
      {
         // might be negativ, such as with:
         // a = val + (CFoo::bar_t)7;
         word_count--;
      }
      last = pc;
      pc   = pc->GetNextNcNnl();
      count++;
   }

   if (  pc->IsNullChunk()
      || chunk_is_not_token(pc, CT_PAREN_CLOSE)
      || chunk_is_token(prev, CT_OC_CLASS))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast, hit type is %s\n",
              __func__, __LINE__, pc->IsNullChunk() ? "Null chunk" : get_token_name(pc->type));
      return;
   }

   if (word_count > 1)
   {
      LOG_FMT(LCASTS, "%s(%d):  -- too many words: %d\n",
              __func__, __LINE__, word_count);
      return;
   }
   paren_close = pc;

   // If last is a type or star/caret, we have a cast for sure
   if (  chunk_is_token(last, CT_STAR)
      || chunk_is_token(last, CT_CARET)
      || chunk_is_token(last, CT_PTR_TYPE)
      || chunk_is_token(last, CT_TYPE)
      || (  chunk_is_token(last, CT_ANGLE_CLOSE)
         && language_is_set(LANG_OC | LANG_JAVA | LANG_CS | LANG_VALA | LANG_CPP)))
   {
      verb = "for sure";
   }
   else if (count == 1)
   {
      /*
       * We are on a potential cast of the form "(word)".
       * We don't know if the word is a type. So lets guess based on some
       * simple rules:
       *  - if all caps, likely a type
       *  - if it ends in _t, likely a type
       *  - if it's objective-c and the type is id, likely valid
       */
      verb = "guessed";

      if (  (last->Len() > 3)
         && (last->str[last->Len() - 2] == '_')
         && (last->str[last->Len() - 1] == 't'))
      {
         detail = " -- '_t'";
      }
      else if (is_ucase_str(last->Text(), last->Len()))
      {
         detail = " -- upper case";
      }
      else if (  language_is_set(LANG_OC)
              && chunk_is_str(last, "id"))
      {
         detail = " -- Objective-C id";
      }
      else
      {
         // If we can't tell for sure whether this is a cast, decide against it
         detail        = " -- mixed case";
         doubtful_cast = true;
      }
      /*
       * If the next item is a * or &, the next item after that can't be a
       * number or string.
       *
       * If the next item is a +, the next item has to be a number.
       *
       * If the next item is a -, the next item can't be a string.
       *
       * For this to be a cast, the close paren must be followed by:
       *  - constant (number or string)
       *  - paren open
       *  - word
       *
       * Find the next non-open paren item.
       */
      pc    = paren_close->GetNextNcNnl();
      after = pc;

      do
      {
         after = after->GetNextNcNnl();
      } while (chunk_is_token(after, CT_PAREN_OPEN));

      if (after->IsNullChunk())
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - hit null chunk\n",
                 __func__, __LINE__);
         return;
      }
      nope = false;

      if (chunk_is_ptr_operator(pc))
      {
         // star (*) and address (&) are ambiguous
         if (  chunk_is_token(after, CT_NUMBER_FP)
            || chunk_is_token(after, CT_NUMBER)
            || chunk_is_token(after, CT_STRING)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (chunk_is_token(pc, CT_MINUS))
      {
         // (UINT8)-1 or (foo)-1 or (FOO)-'a'
         if (  chunk_is_token(after, CT_STRING)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (chunk_is_token(pc, CT_PLUS))
      {
         // (UINT8)+1 or (foo)+1
         if (  (  chunk_is_not_token(after, CT_NUMBER)
               && chunk_is_not_token(after, CT_NUMBER_FP))
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (  chunk_is_not_token(pc, CT_NUMBER_FP)
              && chunk_is_not_token(pc, CT_NUMBER)
              && chunk_is_not_token(pc, CT_WORD)
              && chunk_is_not_token(pc, CT_THIS)
              && chunk_is_not_token(pc, CT_TYPE)
              && chunk_is_not_token(pc, CT_PAREN_OPEN)
              && chunk_is_not_token(pc, CT_STRING)
              && chunk_is_not_token(pc, CT_DECLTYPE)
              && chunk_is_not_token(pc, CT_SIZEOF)
              && get_chunk_parent_type(pc) != CT_SIZEOF
              && chunk_is_not_token(pc, CT_FUNC_CALL)
              && chunk_is_not_token(pc, CT_FUNC_CALL_USER)
              && chunk_is_not_token(pc, CT_FUNCTION)
              && chunk_is_not_token(pc, CT_BRACE_OPEN)
              && (!(  chunk_is_token(pc, CT_SQUARE_OPEN)
                   && language_is_set(LANG_OC))))
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(pc->type));
         return;
      }

      if (nope)
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - Text() '%s' followed by type %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(after->type));
         return;
      }
   }
   // if the 'cast' is followed by a semicolon, comma, bool or close parenthesis, it isn't
   pc = paren_close->GetNextNcNnl();

   if (pc->IsNullChunk())
   {
      return;
   }

   if (  chunk_is_semicolon(pc)
      || chunk_is_token(pc, CT_COMMA)
      || chunk_is_token(pc, CT_BOOL)               // Issue #2151
      || chunk_is_paren_close(pc))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by type %s\n",
              __func__, __LINE__, get_token_name(pc->type));
      return;
   }
   set_chunk_parent(start, CT_C_CAST);
   set_chunk_parent(paren_close, CT_C_CAST);

   LOG_FMT(LCASTS, "%s(%d):  -- %s c-cast: (",
           __func__, __LINE__, verb);

   for (pc = first;
        pc->IsNotNullChunk() && pc != paren_close;
        pc = pc->GetNextNcNnl())
   {
      set_chunk_parent(pc, CT_C_CAST);
      make_type(pc);
      LOG_FMT(LCASTS, " %s", pc->Text());
   }

   LOG_FMT(LCASTS, " )%s\n", detail);

   // Mark the next item as an expression start
   pc = paren_close->GetNextNcNnl();

   if (pc->IsNotNullChunk())
   {
      chunk_flags_set(pc, PCF_EXPR_START);

      if (chunk_is_opening_brace(pc))
      {
         set_paren_parent(pc, get_chunk_parent_type(start));
      }
   }
} // fix_casts


void fix_fcn_def_params(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   LOG_FMT(LFCNP, "%s(%d): Text() '%s', type is %s, on orig_line %zu, level is %zu\n",
           __func__, __LINE__, start->Text(), get_token_name(start->type), start->orig_line, start->level);

   while (  start->IsNotNullChunk()
         && !chunk_is_paren_open(start))
   {
      start = start->GetNextNcNnl();
   }

   if (start->IsNullChunk()) // Coverity CID 76003, 1100782
   {
      return;
   }
   // ensure start chunk holds a single '(' character
   assert(  (start->Len() == 1)
         && (start->str[0] == '('));

   ChunkStack cs;
   size_t     level = start->level + 1;
   Chunk      *pc   = start->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (  (  (start->Len() == 1)
            && (start->str[0] == ')'))
         || pc->level < level)
      {
         LOG_FMT(LFCNP, "%s(%d): bailed on Text() '%s', on orig_line %zu\n",
                 __func__, __LINE__, pc->Text(), pc->orig_line);
         break;
      }
      LOG_FMT(LFCNP, "%s(%d): %s, Text() '%s' on orig_line %zu, level %zu\n",
              __func__, __LINE__, (pc->level > level) ? "skipping" : "looking at",
              pc->Text(), pc->orig_line, pc->level);

      if (pc->level > level)
      {
         pc = pc->GetNextNcNnl();
         continue;
      }

      if (  pc->IsStar()
         || chunk_is_msref(pc)
         || chunk_is_nullable(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
         cs.Push_Back(pc);
      }
      else if (  chunk_is_token(pc, CT_AMP)
              || (  language_is_set(LANG_CPP)
                 && chunk_is_str(pc, "&&")))
      {
         set_chunk_type(pc, CT_BYREF);
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_TYPE_WRAP))
      {
         cs.Push_Back(pc);
      }
      else if (  chunk_is_token(pc, CT_WORD)
              || chunk_is_token(pc, CT_TYPE))
      {
         cs.Push_Back(pc);
      }
      else if (  chunk_is_token(pc, CT_COMMA)
              || chunk_is_token(pc, CT_ASSIGN))
      {
         mark_variable_stack(cs, LFCNP);

         if (chunk_is_token(pc, CT_ASSIGN))
         {
            // Mark assignment for default param spacing
            set_chunk_parent(pc, CT_FUNC_PROTO);
         }
      }
      pc = pc->GetNextNcNnl();
   }
   mark_variable_stack(cs, LFCNP);
} // fix_fcn_def_params


void fix_type_cast(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   Chunk *pc = start->GetNextNcNnl();

   if (  pc->IsNullChunk()
      || chunk_is_not_token(pc, CT_ANGLE_OPEN))
   {
      return;
   }
   pc = pc->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->level >= start->level)
   {
      if (  pc->level == start->level
         && chunk_is_token(pc, CT_ANGLE_CLOSE))
      {
         pc = pc->GetNextNcNnl();

         if (pc->IsNullChunk())
         {
            return;
         }

         if (chunk_is_str(pc, "("))
         {
            set_paren_parent(pc, CT_TYPE_CAST);
         }
         return;
      }
      make_type(pc);
      pc = pc->GetNextNcNnl();
   }
} // fix_type_cast


void fix_typedef(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   LOG_FMT(LTYPEDEF, "%s(%d): typedef @ orig_line %zu, orig_col %zu\n",
           __func__, __LINE__, start->orig_line, start->orig_col);

   Chunk *the_type = Chunk::NullChunkPtr;
   Chunk *last_op  = Chunk::NullChunkPtr;

   /*
    * Mark everything in the typedef and scan for ")(", which makes it a
    * function type
    */
   for (Chunk *next = start->GetNextNcNnl(E_Scope::PREPROC)
        ; next->IsNotNullChunk() && next->level >= start->level
        ; next = next->GetNextNcNnl(E_Scope::PREPROC))
   {
      chunk_flags_set(next, PCF_IN_TYPEDEF);

      if (start->level == next->level)
      {
         if (chunk_is_semicolon(next))
         {
            set_chunk_parent(next, CT_TYPEDEF);
            break;
         }

         if (chunk_is_token(next, CT_ATTRIBUTE))
         {
            break;
         }

         if (  language_is_set(LANG_D)
            && chunk_is_token(next, CT_ASSIGN))
         {
            set_chunk_parent(next, CT_TYPEDEF);
            break;
         }
         make_type(next);

         if (chunk_is_token(next, CT_TYPE))
         {
            the_type = next;
         }
         chunk_flags_clr(next, PCF_VAR_1ST_DEF);

         if (*next->str.c_str() == '(')
         {
            last_op = next;
         }
      }
   }

   // avoid interpreting typedef NS_ENUM (NSInteger, MyEnum) as a function def
   if (  last_op->IsNotNullChunk()
      && !(  language_is_set(LANG_OC)
          && get_chunk_parent_type(last_op) == CT_ENUM))
   {
      flag_parens(last_op, PCF_NONE, CT_FPAREN_OPEN, CT_TYPEDEF, false);
      fix_fcn_def_params(last_op);

      the_type = last_op->GetPrevNcNnlNi(E_Scope::PREPROC);   // Issue #2279

      if (the_type->IsNullChunk())
      {
         return;
      }
      Chunk *open_paren = nullptr;

      if (chunk_is_paren_close(the_type))
      {
         open_paren = chunk_skip_to_match_rev(the_type);
         mark_function_type(the_type);
         the_type = the_type->GetPrevNcNnlNi(E_Scope::PREPROC);   // Issue #2279

         if (the_type->IsNullChunk())
         {
            return;
         }
      }
      else
      {
         // must be: "typedef <return type>func(params);"
         set_chunk_type(the_type, CT_FUNC_TYPE);
      }
      set_chunk_parent(the_type, CT_TYPEDEF);

      LOG_FMT(LTYPEDEF, "%s(%d): fcn typedef Text() '%s', on orig_line %zu\n",
              __func__, __LINE__, the_type->Text(), the_type->orig_line);

      // If we are aligning on the open parenthesis, grab that instead
      log_rule_B("align_typedef_func");

      if (  open_paren != nullptr
         && options::align_typedef_func() == 1)
      {
         the_type = open_paren;
      }
      log_rule_B("align_typedef_func");

      if (options::align_typedef_func() != 0)
      {
         LOG_FMT(LTYPEDEF, "%s(%d):  -- align anchor on Text() %s, @ orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, the_type->Text(), the_type->orig_line, the_type->orig_col);
         chunk_flags_set(the_type, PCF_ANCHOR);
      }
      // already did everything we need to do
      return;
   }
   /*
    * Skip over enum/struct/union stuff, as we know it isn't a return type
    * for a function type
    */
   Chunk *after = start->GetNextNcNnl(E_Scope::PREPROC);

   if (after->IsNullChunk())
   {
      return;
   }

   if (  chunk_is_not_token(after, CT_ENUM)
      && chunk_is_not_token(after, CT_STRUCT)
      && chunk_is_not_token(after, CT_UNION))
   {
      if (  the_type != nullptr
         && the_type->IsNotNullChunk())
      {
         // We have just a regular typedef
         LOG_FMT(LTYPEDEF, "%s(%d): regular typedef Text() %s, on orig_line %zu\n",
                 __func__, __LINE__, the_type->Text(), the_type->orig_line);
         chunk_flags_set(the_type, PCF_ANCHOR);
      }
      return;
   }
   // We have a struct/union/enum, next should be either a type or {
   Chunk *next = after->GetNextNcNnl(E_Scope::PREPROC);

   if (next->IsNullChunk())
   {
      return;
   }

   if (chunk_is_token(next, CT_TYPE))
   {
      next = next->GetNextNcNnl(E_Scope::PREPROC);

      if (next->IsNullChunk())
      {
         return;
      }
   }

   if (chunk_is_token(next, CT_BRACE_OPEN))
   {
      // Skip to the closing brace
      Chunk *br_c = next->GetNextType(CT_BRACE_CLOSE, next->level, E_Scope::PREPROC);

      if (br_c->IsNotNullChunk())
      {
         const E_Token tag = after->type;
         set_chunk_parent(next, tag);
         set_chunk_parent(br_c, tag);

         if (tag == CT_ENUM)
         {
            flag_series(after, br_c, PCF_IN_ENUM);
         }
         else if (tag == CT_STRUCT)
         {
            flag_series(after, br_c, PCF_IN_STRUCT);
         }
      }
   }

   if (  the_type != nullptr
      && the_type->IsNotNullChunk())
   {
      LOG_FMT(LTYPEDEF, "%s(%d): %s typedef Text() %s, on orig_line %zu\n",
              __func__, __LINE__, get_token_name(after->type), the_type->Text(),
              the_type->orig_line);
      chunk_flags_set(the_type, PCF_ANCHOR);
   }
} // fix_typedef


Chunk *fix_variable_definition(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk      *pc = start;
   Chunk      *end;
   Chunk      *tmp_pc;
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
         || chunk_is_token(pc, CT_PP)                       // Issue #3169
         || chunk_is_ptr_operator(pc))
   {
      LOG_FMT(LFVD, "%s(%d):   1:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->type));
      cs.Push_Back(pc);
      pc = pc->GetNextNcNnl();

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFVD, "%s(%d): pc is null chunk\n", __func__, __LINE__);
         return(Chunk::NullChunkPtr);
      }
      LOG_FMT(LFVD, "%s(%d):   2:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->type));

      // Skip templates and attributes
      pc = skip_template_next(pc);

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFVD, "%s(%d): pc is null chunk\n", __func__, __LINE__);
         return(Chunk::NullChunkPtr);
      }
      LOG_FMT(LFVD, "%s(%d):   3:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->type));

      pc = skip_attribute_next(pc);

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFVD, "%s(%d): pc is null chunk\n", __func__, __LINE__);
         return(Chunk::NullChunkPtr);
      }
      LOG_FMT(LFVD, "%s(%d):   4:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->type));

      if (language_is_set(LANG_JAVA))
      {
         pc = skip_tsquare_next(pc);

         if (pc->IsNotNullChunk())
         {
            LOG_FMT(LFVD, "%s(%d):   5:pc->Text() '%s', type is %s\n", __func__, __LINE__, pc->Text(), get_token_name(pc->type));
         }
         else
         {
            pc = Chunk::NullChunkPtr;
         }
      }
   }
   end = pc;

   if (end->IsNullChunk())
   {
      LOG_FMT(LFVD, "%s(%d): end is null chunk\n", __func__, __LINE__);
      return(nullptr);
   }
   LOG_FMT(LFVD, "%s(%d): end->type is %s\n", __func__, __LINE__, get_token_name(end->type));

   if (chunk_is_token(end, CT_FUNC_CTOR_VAR))                // Issue #3010
   {
      return(end);
   }

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

         if (  chunk_is_not_token(tmp_pc, CT_DC_MEMBER)
            && chunk_is_not_token(tmp_pc, CT_MEMBER))
         {
            break;
         }
         idx--;
         tmp_pc = cs.Get(idx)->m_pc;

         if (  chunk_is_not_token(tmp_pc, CT_WORD)
            && chunk_is_not_token(tmp_pc, CT_TYPE))
         {
            break;
         }
         make_type(tmp_pc);
         idx--;
      }
      ref_idx = idx + 1;
   }
   tmp_pc = cs.Get(ref_idx)->m_pc;
   LOG_FMT(LFVD, "%s(%d): ref_idx(%d) is '%s'\n", __func__, __LINE__, ref_idx, tmp_pc->Text());

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
      LOG_FMT(LFVD2, " Text() is '%s', type is %s", tmp_pc->Text(), get_token_name(tmp_pc->type));
   }

   LOG_FMT(LFVD2, "\n");

   // OK we have two or more items, mark types up to the end.
   LOG_FMT(LFVD, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);
   mark_variable_definition(cs.Get(cs.Len() - 1)->m_pc);

   if (chunk_is_token(end, CT_COMMA))
   {
      return(end->GetNextNcNnl());
   }
   return(skip_to_next_statement(end));
} // fix_variable_definition


void mark_cpp_constructor(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *paren_open;
   Chunk *tmp;
   Chunk *after;
   Chunk *var;
   bool  is_destr = false;

   tmp = pc->GetPrevNcNnlNi();   // Issue #2279

   if (  chunk_is_token(tmp, CT_INV)
      || chunk_is_token(tmp, CT_DESTRUCTOR))
   {
      set_chunk_type(tmp, CT_DESTRUCTOR);
      set_chunk_parent(pc, CT_DESTRUCTOR);
      is_destr = true;
   }
   LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, FOUND %sSTRUCTOR for '%s'[%s] prev '%s'[%s]\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col,
           is_destr ? "DE" : "CON",
           pc->Text(), get_token_name(pc->type),
           tmp->Text(), get_token_name(tmp->type));

   paren_open = skip_template_next(pc->GetNextNcNnl());

   if (!chunk_is_str(paren_open, "("))
   {
      LOG_FMT(LWARN, "%s:%zu Expected '(', got: [%s]\n",
              cpd.filename.c_str(), paren_open->orig_line,
              paren_open->Text());
      return;
   }
   // Mark parameters
   fix_fcn_def_params(paren_open);
   after = flag_parens(paren_open, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CLASS_PROTO, false);

   LOG_FMT(LFTOR, "%s(%d): Text() '%s'\n", __func__, __LINE__, after->Text());

   // Scan until the brace open, mark everything
   tmp = paren_open;
   bool hit_colon = false;

   while (  tmp->IsNotNullChunk()
         && (  chunk_is_not_token(tmp, CT_BRACE_OPEN)
            || tmp->level != paren_open->level)
         && !chunk_is_semicolon(tmp))
   {
      LOG_FMT(LFTOR, "%s(%d): tmp is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, tmp->Text(), tmp->orig_line, tmp->orig_col);
      chunk_flags_set(tmp, PCF_IN_CONST_ARGS);
      tmp = tmp->GetNextNcNnl();

      if (  chunk_is_str(tmp, ":")
         && tmp->level == paren_open->level)
      {
         set_chunk_type(tmp, CT_CONSTR_COLON);
         hit_colon = true;
      }

      if (  hit_colon
         && (  chunk_is_paren_open(tmp)
            || chunk_is_opening_brace(tmp))
         && tmp->level == paren_open->level)
      {
         var = skip_template_prev(tmp->GetPrevNcNnlNi());   // Issue #2279

         if (  chunk_is_token(var, CT_TYPE)
            || chunk_is_token(var, CT_WORD))
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
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
   }
   else
   {
      set_chunk_parent(tmp, CT_FUNC_CLASS_PROTO);
      set_chunk_type(pc, CT_FUNC_CLASS_PROTO);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_PROTO on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
   }
   tmp = pc->GetPrevNcNnlNi(); // Issue #2907

   if (chunk_is_token(tmp, CT_DESTRUCTOR))
   {
      set_chunk_parent(tmp, pc->type);
      tmp = tmp->GetPrevNcNnlNi();
   }

   while (chunk_is_token(tmp, CT_QUALIFIER))
   {
      set_chunk_parent(tmp, pc->type);
      tmp = tmp->GetPrevNcNnlNi();
   }
} // mark_cpp_constructor


void mark_cpp_lambda(Chunk *square_open)
{
   if (  chunk_is_token(square_open, CT_SQUARE_OPEN)
      && get_chunk_parent_type(square_open) == CT_CPP_LAMBDA)
   {
      auto *brace_close = square_open->GetNextType(CT_BRACE_CLOSE, square_open->level);

      if (get_chunk_parent_type(brace_close) == CT_CPP_LAMBDA)
      {
         for (auto *pc = square_open; pc != brace_close; pc = pc->GetNextNcNnl())
         {
            chunk_flags_set(pc, PCF_IN_LAMBDA);
         }
      }
   }
} // mark_cpp_lambda


void mark_define_expressions(void)
{
   LOG_FUNC_ENTRY();

   bool  in_define = false;
   bool  first     = true;
   Chunk *pc       = Chunk::GetHead();
   Chunk *prev     = pc;

   while (pc->IsNotNullChunk())
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
         if (  !pc->flags.test(PCF_IN_PREPROC)
            || chunk_is_token(pc, CT_PREPROC))
         {
            in_define = false;
         }
         else
         {
            if (  chunk_is_not_token(pc, CT_MACRO)
               && (  first
                  || chunk_is_token(prev, CT_PAREN_OPEN)
                  || chunk_is_token(prev, CT_ARITH)
                  || chunk_is_token(prev, CT_SHIFT)
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
      pc   = pc->GetNext();
   }
} // mark_define_expressions


void mark_exec_sql(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *tmp;

   // Change CT_WORD to CT_SQL_WORD
   for (tmp = pc->GetNext(); tmp != nullptr && tmp->IsNotNullChunk(); tmp = tmp->GetNext())
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

   if (  chunk_is_not_token(pc, CT_SQL_BEGIN)
      || tmp->IsNullChunk()
      || chunk_is_not_token(tmp, CT_SEMICOLON))
   {
      return;
   }

   for (tmp = tmp->GetNext();
        tmp->IsNotNullChunk() && chunk_is_not_token(tmp, CT_SQL_END);
        tmp = tmp->GetNext())
   {
      tmp->level++;
   }
} // mark_exec_sql


void mark_function_return_type(Chunk *fname, Chunk *start, E_Token parent_type)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = start;

   if (pc != nullptr)
   {
      // Step backwards from pc and mark the parent of the return type
      LOG_FMT(LFCNR, "%s(%d): (backwards) return type for '%s' @ orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, fname->Text(), fname->orig_line, fname->orig_col);

      Chunk *first = pc;

      while (pc->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s, ",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         log_pcf_flags(LFCNR, pc->flags);

         if (chunk_is_token(pc, CT_ANGLE_CLOSE))
         {
            pc = skip_template_prev(pc);

            if (  pc->IsNullChunk()
               || chunk_is_token(pc, CT_TEMPLATE))
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
               && chunk_is_not_token(pc, CT_OPERATOR)
               && chunk_is_not_token(pc, CT_WORD)
               && chunk_is_not_token(pc, CT_ADDR))
            || pc->flags.test(PCF_IN_PREPROC))
         {
            break;
         }

         if (!chunk_is_ptr_operator(pc))
         {
            first = pc;
         }
         pc = pc->GetPrevNcNnlNi();   // Issue #2279
      }
      LOG_FMT(LFCNR, "%s(%d): marking returns...", __func__, __LINE__);

      // Changing words to types into tuple return types in CS.
      bool is_return_tuple = false;

      if (  chunk_is_token(pc, CT_PAREN_CLOSE)
         && !pc->flags.test(PCF_IN_PREPROC))
      {
         first           = chunk_skip_to_match_rev(pc);
         is_return_tuple = true;
      }
      pc = first;

      while (pc->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, " Text() '%s', type is %s", pc->Text(), get_token_name(pc->type));

         if (parent_type != CT_NONE)
         {
            set_chunk_parent(pc, parent_type);
         }
         Chunk *prev = pc->GetPrevNcNnlNi();   // Issue #2279

         if (  !is_return_tuple
            || chunk_is_not_token(pc, CT_WORD)
            || (  prev->IsNullChunk()
               && chunk_is_not_token(prev, CT_TYPE)))
         {
            make_type(pc);
         }

         if (pc == start)
         {
            break;
         }
         pc = pc->GetNextNcNnl();

         //template angles should keep parent type CT_TEMPLATE
         if (chunk_is_token(pc, CT_ANGLE_OPEN))
         {
            pc = pc->GetNextType(CT_ANGLE_CLOSE, pc->level);

            if (pc == start)
            {
               break;
            }
            pc = pc->GetNextNcNnl();
         }
      }
      LOG_FMT(LFCNR, "\n");

      // Back up and mark parent type on friend declarations
      if (  parent_type != CT_NONE
         && first
         && first->flags.test(PCF_IN_CLASS))
      {
         pc = first->GetPrevNcNnlNi();   // Issue #2279

         if (chunk_is_token(pc, CT_FRIEND))
         {
            LOG_FMT(LFCNR, "%s(%d): marking friend\n", __func__, __LINE__);
            set_chunk_parent(pc, parent_type);
            // A friend might be preceded by a template specification, as in:
            //   template <...> friend type func(...);
            // If so, we need to mark that also
            pc = pc->GetPrevNcNnlNi();   // Issue #2279

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


void mark_function(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
   Chunk *prev = pc->GetPrevNcNnlNi();   // Issue #2279
   Chunk *next = pc->GetNextNppOrNcNnl();

   if (next->IsNullChunk())
   {
      return;
   }
   Chunk *tmp;
   Chunk *semi = nullptr;
   Chunk *paren_open;
   Chunk *paren_close;

   // Find out what is before the operator
   if (get_chunk_parent_type(pc) == CT_OPERATOR)
   {
      LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      log_pcf_flags(LGUY, pc->flags);
      Chunk *pc_op = pc->GetPrevType(CT_OPERATOR, pc->level);

      if (  pc_op->IsNotNullChunk()
         && pc_op->flags.test(PCF_EXPR_START))
      {
         LOG_FMT(LFCN, "%s(%d): (4) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
         set_chunk_type(pc, CT_FUNC_CALL);
      }

      if (language_is_set(LANG_CPP))
      {
         tmp = pc;

         while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
         {
            if (  chunk_is_token(tmp, CT_BRACE_CLOSE)
               || chunk_is_token(tmp, CT_BRACE_OPEN)             // Issue 575
               || chunk_is_token(tmp, CT_SEMICOLON))
            {
               break;
            }

            if (  chunk_is_paren_open(tmp)
               && !pc->flags.test(PCF_IN_PREPROC))               // Issue #2703
            {
               LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->Text());
               LOG_FMT(LFCN, "%s(%d): (5) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }

            if (chunk_is_token(tmp, CT_ASSIGN))
            {
               LOG_FMT(LFCN, "%s(%d): (6) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }

            if (chunk_is_token(tmp, CT_TEMPLATE))
            {
               LOG_FMT(LFCN, "%s(%d): (7) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
               set_chunk_type(pc, CT_FUNC_DEF);
               break;
            }

            if (chunk_is_token(tmp, CT_BRACE_OPEN))
            {
               if (get_chunk_parent_type(tmp) == CT_FUNC_DEF)
               {
                  LOG_FMT(LFCN, "%s(%d): (8) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
                  set_chunk_type(pc, CT_FUNC_CALL);
               }

               if (  get_chunk_parent_type(tmp) == CT_CLASS
                  || get_chunk_parent_type(tmp) == CT_STRUCT)
               {
                  LOG_FMT(LFCN, "%s(%d): (9) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
                  set_chunk_type(pc, CT_FUNC_DEF);
               }
               break;
            }
         }

         if (  tmp != nullptr
            && chunk_is_not_token(pc, CT_FUNC_CALL))
         {
            // Mark the return type
            tmp = tmp->GetNextNcNnl();

            while (  tmp != pc
                  && tmp->IsNotNullChunk())
            {
               make_type(tmp); // Mark the return type
               tmp = tmp->GetNextNcNnl();
            }
         }
      }
   }

   if (  chunk_is_ptr_operator(next)
      || chunk_is_newline(next))
   {
      next = next->GetNextNppOrNcNnl();

      if (next->IsNullChunk())
      {
         return;
      }
   }
   LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s, type is %s, parent_type is %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)));
   LOG_FMT(LFCN, "   level is %zu, brace_level is %zu, next->Text() '%s', next->type is %s, next->level is %zu\n",
           pc->level, pc->brace_level,
           next->Text(), get_token_name(next->type), next->level);

   if (pc->flags.test(PCF_IN_CONST_ARGS))
   {
      set_chunk_type(pc, CT_FUNC_CTOR_VAR);
      LOG_FMT(LFCN, "%s(%d):   1) Marked [%s] as FUNC_CTOR_VAR on line %zu col %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
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
   paren_open  = pc->GetNextString("(", 1, pc->level);
   paren_close = paren_open->GetNextString(")", 1, pc->level);

   if (  paren_open->IsNullChunk()
      || paren_close->IsNullChunk())
   {
      LOG_FMT(LFCN, "%s(%d): No parens found for [%s] on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
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
   tmp = paren_close->GetNextNcNnl();

   if (  tmp->IsNotNullChunk()
      && chunk_is_str(tmp, "("))
   {
      Chunk *tmp1;
      Chunk *tmp2;
      Chunk *tmp3;

      // skip over any leading class/namespace in: "T(F::*A)();"
      tmp1 = next->GetNextNcNnl();

      while (tmp1->IsNotNullChunk())
      {
         tmp2 = tmp1->GetNextNcNnl();

         if (  !chunk_is_word(tmp1)
            || chunk_is_not_token(tmp2, CT_DC_MEMBER))
         {
            break;
         }
         tmp1 = tmp2->GetNextNcNnl();
      }
      tmp2 = tmp1->GetNextNcNnl();

      if (chunk_is_str(tmp2, ")"))
      {
         tmp3 = tmp2;
         tmp2 = Chunk::NullChunkPtr;
      }
      else
      {
         tmp3 = tmp2->GetNextNcNnl();
      }
      tmp3 = chunk_get_next_ssq(tmp3);

      if (  chunk_is_str(tmp3, ")")
         && (  tmp1->IsStar()
            || chunk_is_msref(tmp1)
            || (  language_is_set(LANG_OC)
               && chunk_is_token(tmp1, CT_CARET)))
         && (  tmp2->IsNullChunk()
            || chunk_is_token(tmp2, CT_WORD)))
      {
         if (tmp2->IsNotNullChunk())
         {
            LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, function variable '%s', changing '%s' into a type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, tmp2->Text(), pc->Text());
            set_chunk_type(tmp2, CT_FUNC_VAR);
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_VAR, false);

            LOG_FMT(LFCN, "%s(%d): paren open @ orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, paren_open->orig_line, paren_open->orig_col);
         }
         else
         {
            LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, function type, changing '%s' into a type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

            if (tmp2)
            {
               set_chunk_type(tmp2, CT_FUNC_TYPE);
            }
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_TYPE, false);
         }
         set_chunk_type(pc, CT_TYPE);
         set_chunk_type(tmp1, CT_PTR_TYPE);
         chunk_flags_clr(pc, PCF_VAR_1ST_DEF);

         if (tmp2->IsNotNullChunk())
         {
            chunk_flags_set(tmp2, PCF_VAR_1ST_DEF);
         }
         flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_PROTO, false);
         fix_fcn_def_params(tmp);
         return;
      }
      LOG_FMT(LFCN, "%s(%d): chained function calls? Text() is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
   }

   // Assume it is a function call if not already labeled
   if (chunk_is_token(pc, CT_FUNCTION))
   {
      LOG_FMT(LFCN, "%s(%d): examine: Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
      // look for an assigment. Issue #575
      Chunk *temp = pc->GetNextType(CT_ASSIGN, pc->level);

      if (temp->IsNotNullChunk())
      {
         LOG_FMT(LFCN, "%s(%d): assigment found, orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, temp->orig_line, temp->orig_col, temp->Text());
         LOG_FMT(LFCN, "%s(%d): (10) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): (11) SET TO %s: orig_line is %zu, orig_col is %zu, Text() '%s'",
                 __func__, __LINE__, (get_chunk_parent_type(pc) == CT_OPERATOR) ? "CT_FUNC_DEF" : "CT_FUNC_CALL",
                 pc->orig_line, pc->orig_col, pc->Text());
         set_chunk_type(pc, (get_chunk_parent_type(pc) == CT_OPERATOR) ? CT_FUNC_DEF : CT_FUNC_CALL);
      }
   }
   LOG_FMT(LFCN, "%s(%d): Check for C++ function def, Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
           __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));

   if (prev != nullptr)
   {
      LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));
   }
   else
   {
      prev = Chunk::NullChunkPtr;
   }

   // Check for C++ function def
   if (  chunk_is_token(pc, CT_FUNC_CLASS_DEF)
      || (  prev->IsNotNullChunk()
         && (  chunk_is_token(prev, CT_INV)
            || chunk_is_token(prev, CT_DC_MEMBER))))
   {
      Chunk *destr = Chunk::NullChunkPtr;

      if (chunk_is_token(prev, CT_INV))
      {
         // TODO: do we care that this is the destructor?
         set_chunk_type(prev, CT_DESTRUCTOR);
         set_chunk_type(pc, CT_FUNC_CLASS_DEF);

         set_chunk_parent(pc, CT_DESTRUCTOR);

         destr = prev;
         // Point to the item previous to the class name
         prev = prev->GetPrevNcNnlNpp();
      }

      if (chunk_is_token(prev, CT_DC_MEMBER))
      {
         prev = prev->GetPrevNcNnlNpp();

         if (prev->IsNotNullChunk())
         {
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col,
                    get_token_name(prev->type));
            prev = skip_template_prev(prev);
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col,
                    get_token_name(prev->type));
            prev = skip_attribute_prev(prev);
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col,
                    get_token_name(prev->type));
         }

         if (  chunk_is_token(prev, CT_WORD)
            || chunk_is_token(prev, CT_TYPE))
         {
            if (pc->str.equals(prev->str))
            {
               LOG_FMT(LFCN, "%s(%d): pc->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col,
                       get_token_name(prev->type));
               set_chunk_type(pc, CT_FUNC_CLASS_DEF);
               LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu - FOUND %sSTRUCTOR for '%s', type is %s\n",
                       __func__, __LINE__,
                       prev->orig_line, prev->orig_col,
                       (destr->IsNotNullChunk()) ? "DE" : "CON",
                       prev->Text(), get_token_name(prev->type));

               mark_cpp_constructor(pc);
               return;
            }
            // Point to the item previous to the class name
            prev = prev->GetPrevNcNnlNpp();
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
      LOG_FMT(LFCN, "%s(%d): pc->Text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col,
              get_token_name(pc->type));

      if (prev->IsNullChunk())
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev is null chunk\n",
                 __func__, __LINE__);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev->Text() '%s', prev->type is %s\n",
                 __func__, __LINE__, prev->Text(), get_token_name(prev->type));
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
      while (prev->IsNotNullChunk())
      {
         LOG_FMT(LFCN, "%s(%d): next step with: prev->orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, prev->orig_line, prev->orig_col, prev->Text());

         if (get_chunk_parent_type(pc) == CT_FIXED)
         {
            isa_def = true;
         }

         if (prev->flags.test(PCF_IN_PREPROC))
         {
            prev = prev->GetPrevNcNnlNpp();
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

            if (prev != nullptr)
            {
               prev = prev->GetPrev();
            }

            if (chunk_is_token(prev, CT_DECLSPEC))
            {
               if (  prev != nullptr
                  && prev->IsNotNullChunk())
               {
                  prev = prev->GetPrev();
               }
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
               prev = prev->GetPrevNcNnlNpp();

               if (  prev->IsNullChunk()
                  || (  chunk_is_not_token(prev, CT_WORD)
                     && chunk_is_not_token(prev, CT_TYPE)
                     && chunk_is_not_token(prev, CT_THIS)))
               {
                  LOG_FMT(LFCN, "%s(%d): --? skipped MEMBER and landed on %s\n",
                          __func__, __LINE__, (prev->IsNullChunk()) ? "<null chunk>" : get_token_name(prev->type));
                  break;
               }
               LOG_FMT(LFCN, "%s(%d): <skip> '%s'\n",
                       __func__, __LINE__, prev->Text());

               // Issue #1112
               // clarification: this will skip the CT_WORD, CT_TYPE or CT_THIS landing on either
               // another CT_DC_MEMBER or CT_MEMBER or a token that indicates the context of the
               // token in question; therefore, exit loop when not a CT_DC_MEMBER or CT_MEMBER
               prev = prev->GetPrevNcNnlNpp();

               if (prev->IsNullChunk())
               {
                  LOG_FMT(LFCN, "%s(%d): prev is null chunk\n",
                          __func__, __LINE__);
               }
               else
               {
                  LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                          __func__, __LINE__, prev->orig_line, prev->orig_col, prev->Text());
               }
            }

            if (prev->IsNullChunk())
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
            Chunk *prev_prev = prev->GetPrevNcNnlNpp();

            if (!chunk_is_token(prev_prev, CT_QUESTION))               // Issue #1753
            {
               LOG_FMT(LFCN, "%s(%d):   --> maybe a proto/def\n",
                       __func__, __LINE__);

               LOG_FMT(LFCN, "%s(%d): prev is '%s', orig_line is %zu, orig_col is %zu, type is %s, parent_type is %s\n",
                       __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col,
                       get_token_name(prev->type), get_token_name(get_chunk_parent_type(prev)));
               log_pcf_flags(LFCN, pc->flags);
               isa_def = true;
            }
         }

         if (chunk_is_ptr_operator(prev))
         {
            hit_star = true;
         }

         if (  chunk_is_not_token(prev, CT_OPERATOR)
            && chunk_is_not_token(prev, CT_TSQUARE)
            && chunk_is_not_token(prev, CT_ANGLE_CLOSE)
            && chunk_is_not_token(prev, CT_QUALIFIER)
            && chunk_is_not_token(prev, CT_TYPE)
            && chunk_is_not_token(prev, CT_WORD)
            && !chunk_is_ptr_operator(prev))
         {
            LOG_FMT(LFCN, "%s(%d):  --> Stopping on prev is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));

            // certain tokens are unlikely to precede a prototype or definition
            if (  chunk_is_token(prev, CT_ARITH)
               || chunk_is_token(prev, CT_SHIFT)
               || chunk_is_token(prev, CT_ASSIGN)
               || chunk_is_token(prev, CT_COMMA)
               || (  chunk_is_token(prev, CT_STRING)
                  && get_chunk_parent_type(prev) != CT_EXTERN)  // fixes issue 1259
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
            prev = prev->GetPrevNcNnlNpp();
         }
      }
      //LOG_FMT(LFCN, " -- stopped on %s [%s]\n",
      //        prev->Text(), get_token_name(prev->type));

      // Fixes issue #1634
      if (chunk_is_paren_close(prev))
      {
         Chunk *preproc = prev->GetNextNcNnl();

         if (chunk_is_token(preproc, CT_PREPROC))
         {
            size_t pp_level = preproc->pp_level;

            if (chunk_is_token(preproc->GetNextNcNnl(), CT_PP_ELSE))
            {
               do
               {
                  preproc = preproc->GetPrevNcNnlNi();      // Issue #2279

                  if (chunk_is_token(preproc, CT_PP_IF))
                  {
                     preproc = preproc->GetPrevNcNnlNi();   // Issue #2279

                     if (preproc->pp_level == pp_level)
                     {
                        prev = preproc->GetPrevNcNnlNpp();
                        break;
                     }
                  }
               } while (preproc->IsNotNullChunk());
            }
         }
      }

      if (  isa_def
         && prev != nullptr
         && prev->IsNotNullChunk()
         && (  (  chunk_is_paren_close(prev)
               && get_chunk_parent_type(prev) != CT_D_CAST
               && get_chunk_parent_type(prev) != CT_MACRO_OPEN  // Issue #2726
               && get_chunk_parent_type(prev) != CT_MACRO_CLOSE)
            || chunk_is_token(prev, CT_ASSIGN)
            || chunk_is_token(prev, CT_RETURN)))
      {
         LOG_FMT(LFCN, "%s(%d): -- overriding DEF due to prev is '%s', type is %s\n",
                 __func__, __LINE__, prev->Text(), get_token_name(prev->type));
         isa_def = false;
      }

      // Fixes issue #1266, identification of a tuple return type in CS.
      if (  !isa_def
         && chunk_is_token(prev, CT_PAREN_CLOSE)
         && prev->GetNextNcNnl() == pc)
      {
         tmp = chunk_skip_to_match_rev(prev);

         if (tmp == nullptr)
         {
            tmp = Chunk::NullChunkPtr;
         }

         while (  tmp->IsNotNullChunk() // Issue #2315
               && tmp != prev)
         {
            if (  chunk_is_token(tmp, CT_COMMA)
               && tmp->level == prev->level + 1)
            {
               LOG_FMT(LFCN, "%s(%d): -- overriding call due to tuple return type -- prev is '%s', type is %s\n",
                       __func__, __LINE__, prev->Text(), get_token_name(prev->type));
               isa_def = true;
               break;
            }
            tmp = tmp->GetNextNcNnl();
         }
      }

      if (isa_def)
      {
         LOG_FMT(LFCN, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
         LOG_FMT(LFCN, "%s(%d): (12) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
         set_chunk_type(pc, CT_FUNC_DEF);

         if (  prev == nullptr
            || prev->IsNullChunk())
         {
            prev = Chunk::GetHead();
         }

         for (tmp = prev; tmp->IsNotNullChunk() && tmp != pc; tmp = tmp->GetNextNcNnlNpp())
         {
            LOG_FMT(LFCN, "%s(%d): Text() is '%s', type is %s\n",
                    __func__, __LINE__, tmp->Text(), get_token_name(tmp->type));
            make_type(tmp);
         }
      }
   }

   if (chunk_is_not_token(pc, CT_FUNC_DEF))
   {
      LOG_FMT(LFCN, "%s(%d):  Detected type %s, Text() is '%s', on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, get_token_name(pc->type),
              pc->Text(), pc->orig_line, pc->orig_col);

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
   tmp = paren_close->GetNextNcNnl();

   while (tmp->IsNotNullChunk())
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
            LOG_FMT(LFCN, "%s(%d):   2) Marked Text() is '%s', as FUNC_PROTO on orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
            break;
         }
         else if (chunk_is_token(pc, CT_COMMA))
         {
            set_chunk_type(pc, CT_FUNC_CTOR_VAR);
            LOG_FMT(LFCN, "%s(%d):   2) Marked Text() is '%s', as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
            break;
         }
      }
      tmp = tmp->GetNextNcNnl();
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
              pc->Text(),
              get_token_name(paren_open->type),
              get_token_name(paren_close->type));

      /*
       * Check the token at the start of the statement. If it's 'extern', we
       * definitely have a function prototype.
       */
      tmp = pc;

      while (  tmp->IsNotNullChunk()
            && !tmp->flags.test(PCF_STMT_START))
      {
         tmp = tmp->GetPrevNcNnlNi();   // Issue #2279
      }
      const bool is_extern = (  tmp->IsNotNullChunk()
                             && tmp->str.equals("extern"));

      /*
       * Scan the parameters looking for:
       *  - constant strings
       *  - numbers
       *  - non-type fields
       *  - function calls
       */
      Chunk *ref = paren_open->GetNextNcNnl();
      Chunk *tmp2;
      bool  is_param = true;
      tmp = ref;

      while (tmp != paren_close)
      {
         tmp2 = tmp->GetNextNcNnl();

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
         && is_param
         && ref != tmp)
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
         LOG_FMT(LFCN, "%s(%d):   3) Marked Text() '%s' as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
      }
      else if (pc->brace_level > 0)
      {
         Chunk *br_open = pc->GetPrevType(CT_BRACE_OPEN, pc->brace_level - 1);

         if (  br_open->IsNotNullChunk()
            && get_chunk_parent_type(br_open) != CT_EXTERN
            && get_chunk_parent_type(br_open) != CT_NAMESPACE)
         {
            // Do a check to see if the level is right
            prev = pc->GetPrevNcNnlNi();   // Issue #2279

            if (  !chunk_is_str(prev, "*")
               && !chunk_is_str(prev, "&"))
            {
               Chunk *p_op = pc->GetPrevType(CT_BRACE_OPEN, pc->brace_level - 1);

               if (  p_op->IsNotNullChunk()
                  && get_chunk_parent_type(p_op) != CT_CLASS
                  && get_chunk_parent_type(p_op) != CT_STRUCT
                  && get_chunk_parent_type(p_op) != CT_NAMESPACE)
               {
                  set_chunk_type(pc, CT_FUNC_CTOR_VAR);
                  LOG_FMT(LFCN, "%s(%d):   4) Marked Text() is'%s', as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                          __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
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
      next = next->GetNextNcNnl();

      if (next->IsNullChunk())
      {
         return;
      }
   }
   // Mark parameters and return type
   fix_fcn_def_params(next);
   mark_function_return_type(pc, pc->GetPrevNcNnlNi(), pc->type);   // Issue #2279

   /* mark C# where chunk */
   if (  language_is_set(LANG_CS)
      && (  (chunk_is_token(pc, CT_FUNC_DEF))
         || (chunk_is_token(pc, CT_FUNC_PROTO))))
   {
      tmp = paren_close->GetNextNcNnl();
      pcf_flags_t in_where_spec_flags = PCF_NONE;

      while (  tmp->IsNotNullChunk()
            && chunk_is_not_token(tmp, CT_BRACE_OPEN)
            && chunk_is_not_token(tmp, CT_SEMICOLON))
      {
         mark_where_chunk(tmp, pc->type, tmp->flags | in_where_spec_flags);
         in_where_spec_flags = tmp->flags & PCF_IN_WHERE_SPEC;

         tmp = tmp->GetNextNcNnl();
      }
   }

   // Find the brace pair and set the parent
   if (chunk_is_token(pc, CT_FUNC_DEF))
   {
      tmp = paren_close->GetNextNcNnl();

      while (  tmp->IsNotNullChunk()
            && chunk_is_not_token(tmp, CT_BRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): (13) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->Text());
         set_chunk_parent(tmp, CT_FUNC_DEF);

         if (!chunk_is_semicolon(tmp))
         {
            chunk_flags_set(tmp, PCF_OLD_FCN_PARAMS);
         }
         tmp = tmp->GetNextNcNnl();
      }

      if (chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): (14) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->Text());
         set_chunk_parent(tmp, CT_FUNC_DEF);
         tmp = chunk_skip_to_match(tmp);

         if (tmp != nullptr)
         {
            LOG_FMT(LFCN, "%s(%d): (15) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                    __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->Text());
            set_chunk_parent(tmp, CT_FUNC_DEF);
         }
      }
   }
} // mark_function


bool mark_function_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTYPE, "%s(%d): type is %s, Text() '%s' @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->Text(),
           pc->orig_line, pc->orig_col);

   size_t  star_count = 0;
   size_t  word_count = 0;
   Chunk   *ptrcnk    = nullptr;
   Chunk   *tmp;
   Chunk   *apo;
   Chunk   *apc;
   Chunk   *aft;
   bool    anon = false;
   E_Token pt, ptp;

   // Scan backwards across the name, which can only be a word and single star
   Chunk *varcnk = pc->GetPrevNcNnlNi();   // Issue #2279

   varcnk = chunk_get_prev_ssq(varcnk);

   if (  varcnk->IsNotNullChunk()
      && !chunk_is_word(varcnk))
   {
      if (  language_is_set(LANG_OC)
         && chunk_is_str(varcnk, "^")
         && chunk_is_paren_open(varcnk->GetPrevNcNnlNi()))   // Issue #2279
      {
         // anonymous ObjC block type -- RTYPE (^)(ARGS)
         anon = true;
      }
      else
      {
         LOG_FMT(LFTYPE, "%s(%d): not a word: Text() '%s', type is %s, @ orig_line is %zu:, orig_col is %zu\n",
                 __func__, __LINE__, varcnk->Text(), get_token_name(varcnk->type),
                 varcnk->orig_line, varcnk->orig_col);
         goto nogo_exit;
      }
   }
   apo = pc->GetNextNcNnl();

   if (apo->IsNullChunk())
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
   aft = apc->GetNextNcNnl();

   if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      pt = CT_FUNC_DEF;
   }
   else if (  chunk_is_token(aft, CT_SEMICOLON)
           || chunk_is_token(aft, CT_ASSIGN))
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

   while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
   {
      tmp = chunk_get_prev_ssq(tmp);

      LOG_FMT(LFTYPE, " -- type is %s, %s on orig_line %zu, orig_col is %zu",
              get_token_name(tmp->type), tmp->Text(),
              tmp->orig_line, tmp->orig_col);

      if (  tmp->IsStar()
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
         LOG_FMT(LFTYPE, " -- TYPE(%s)\n", tmp->Text());
      }
      else if (chunk_is_token(tmp, CT_DC_MEMBER))
      {
         word_count = 0;
         LOG_FMT(LFTYPE, " -- :: reset word_count\n");
      }
      else if (chunk_is_str(tmp, "("))
      {
         LOG_FMT(LFTYPE, " -- open paren (break)\n");
         break;
      }
      else
      {
         LOG_FMT(LFTYPE, " --  unexpected token: type is %s, Text() '%s', on orig_line %zu, orig_col %zu\n",
                 get_token_name(tmp->type), tmp->Text(),
                 tmp->orig_line, tmp->orig_col);
         goto nogo_exit;
      }
   }

   // Fixes #issue 1577
   // Allow word count 2 incase of function pointer declaration.
   // Ex: bool (__stdcall* funcptr)(int, int);
   if (  star_count > 1
      || (  word_count > 1
         && !(  word_count == 2
             && ptp == CT_FUNC_VAR))
      || ((star_count + word_count) == 0))
   {
      LOG_FMT(LFTYPE, "%s(%d): bad counts word: %zu, star: %zu\n",
              __func__, __LINE__, word_count, star_count);
      goto nogo_exit;
   }

   // make sure what appears before the first open paren can be a return type
   if (!chunk_ends_type(tmp->GetPrevNcNnlNi()))   // Issue #2279
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
         set_chunk_type(varcnk, CT_FUNC_TYPE);   // Issue #3402
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

   while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
   {
      LOG_FMT(LFTYPE, " ++ type is %s, Text() '%s', on orig_line %zu, orig_col %zu\n",
              get_token_name(tmp->type), tmp->Text(),
              tmp->orig_line, tmp->orig_col);

      if (*tmp->str.c_str() == '(')
      {
         if (!pc->flags.test(PCF_IN_TYPEDEF))
         {
            chunk_flags_set(tmp, PCF_VAR_1ST_DEF);
         }
         set_chunk_type(tmp, CT_TPAREN_OPEN);
         set_chunk_parent(tmp, ptp);

         tmp = tmp->GetPrevNcNnlNi();   // Issue #2279

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
   tmp = pc->GetNextNcNnl();

   if (chunk_is_paren_open(tmp))
   {
      LOG_FMT(LFTYPE, "%s(%d): setting FUNC_CALL on orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col);
      flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
   }
   return(false);
} // mark_function_type


void mark_lvalue(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *prev;

   if (pc->flags.test(PCF_IN_PREPROC))
   {
      return;
   }

   for (prev = pc->GetPrevNcNnlNi();     // Issue #2279
        prev->IsNotNullChunk();
        prev = prev->GetPrevNcNnlNi())   // Issue #2279
   {
      if (  prev->level < pc->level
         || chunk_is_token(prev, CT_ACCESS_COLON)
         || chunk_is_token(prev, CT_ASSIGN)
         || chunk_is_token(prev, CT_BOOL)
         || chunk_is_token(prev, CT_COMMA)
         || chunk_is_cpp_inheritance_access_specifier(prev)
         || chunk_is_semicolon(prev)
         || chunk_is_str(prev, "(")
         || chunk_is_str(prev, "{")
         || chunk_is_str(prev, "[")
         || prev->flags.test(PCF_IN_PREPROC)
         || get_chunk_parent_type(prev) == CT_NAMESPACE
         || get_chunk_parent_type(prev) == CT_TEMPLATE)
      {
         break;
      }
      chunk_flags_set(prev, PCF_LVALUE);

      if (  prev->level == pc->level
         && chunk_is_str(prev, "&"))
      {
         make_type(prev);
      }
   }
} // mark_lvalue


void mark_struct_union_body(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = start;

   while (  pc->IsNotNullChunk()
         && pc->level >= start->level
         && !(  pc->level == start->level
             && chunk_is_token(pc, CT_BRACE_CLOSE)))
   {
      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_SEMICOLON))
      {
         pc = pc->GetNextNcNnl();

         if (pc->IsNullChunk())
         {
            break;
         }
      }

      if (chunk_is_token(pc, CT_ALIGN))
      {
         pc = skip_align(pc); // "align(x)" or "align(x):"

         if (pc->IsNullChunk())
         {
            break;
         }
      }
      else if (chunk_is_token(pc, CT_AMP))
      {
         pc = skip_expression(pc);
      }
      else
      {
         pc = fix_variable_definition(pc);

         if (pc->IsNullChunk())
         {
            break;
         }
      }
   }
} // mark_struct_union_body


void mark_template_func(Chunk *pc, Chunk *pc_next)
{
   LOG_FUNC_ENTRY();

   // We know angle_close must be there...
   Chunk *angle_close = pc_next->GetNextType(CT_ANGLE_CLOSE, pc->level);
   Chunk *after       = angle_close->GetNextNcNnl();

   if (after->IsNotNullChunk())
   {
      if (chunk_is_str(after, "("))
      {
         if (angle_close->flags.test(PCF_IN_FCN_CALL))
         {
            LOG_FMT(LTEMPFUNC, "%s(%d): marking '%s' in line %zu as a FUNC_CALL\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line);
            LOG_FMT(LFCN, "%s(%d): (16) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
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
                    __func__, __LINE__, pc->Text(), pc->orig_line);
            // its a function!!!
            LOG_FMT(LFCN, "%s(%d): (17) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
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


Chunk *mark_variable_definition(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return(nullptr);
   }
   Chunk       *pc   = start;
   pcf_flags_t flags = PCF_VAR_1ST_DEF;

   LOG_FMT(LVARDEF, "%s(%d): orig_line %zu, orig_col %zu, Text() '%s', type is %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(),
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
         LOG_FMT(LVARDEF, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', set PCF_VAR_1ST\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

         LOG_FMT(LVARDEF,
                 "%s(%d): orig_line is %zu, marked Text() '%s'[%s]\n"
                 "   in orig_col %zu, flags: %s -> %s\n",
                 __func__, __LINE__, pc->orig_line, pc->Text(),
                 get_token_name(pc->type), pc->orig_col,
                 pcf_flags_str(orig_flags).c_str(),
                 pcf_flags_str(pc->flags).c_str());
      }
      else if (  !bit_field_colon_is_present                      // Issue #2689
              && (  pc->IsStar()
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
      pc = pc->GetNextNcNnl();
   }
   return(pc);
} // mark_variable_definition


void mark_variable_stack(ChunkStack &cs, log_sev_t sev)
{
   UNUSED(sev);
   LOG_FUNC_ENTRY();

   // throw out the last word and mark the rest
   Chunk *var_name = cs.Pop_Back();

   if (  var_name != nullptr
      && var_name->GetPrev()->IsNotNullChunk()
      && var_name->GetPrev()->type == CT_DC_MEMBER)
   {
      cs.Push_Back(var_name);
   }

   if (var_name != nullptr)
   {
      LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu:\n",
              __func__, __LINE__, var_name->orig_line, var_name->orig_col);

      size_t word_cnt = 0;
      Chunk  *word_type;

      while ((word_type = cs.Pop_Back()) != nullptr)
      {
         if (  chunk_is_token(word_type, CT_WORD)
            || chunk_is_token(word_type, CT_TYPE))
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, word_type->Text());
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
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, var_name->Text());
            chunk_flags_set(var_name, PCF_VAR_DEF);
         }
         else
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, var_name->Text());
            set_chunk_type(var_name, CT_TYPE);
            chunk_flags_set(var_name, PCF_VAR_TYPE);
         }
      }
   }
} // mark_variable_stack


pcf_flags_t mark_where_chunk(Chunk *pc, E_Token parent_type, pcf_flags_t flags)
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
      if (chunk_is_str(pc, ":"))
      {
         set_chunk_type(pc, CT_WHERE_COLON);
         LOG_FMT(LFTOR, "%s: where-spec colon on line %zu\n",
                 __func__, pc->orig_line);
      }
      else if (  (chunk_is_token(pc, CT_STRUCT))
              || (chunk_is_token(pc, CT_CLASS)))
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
} // mark_where_chunk
