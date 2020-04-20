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
#include "lang_pawn.h"
#include "log_rules.h"


void fix_casts(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc;
   chunk_t    *prev;
   chunk_t    *first;
   chunk_t    *after;
   chunk_t    *last = nullptr;
   chunk_t    *paren_close;
   const char *verb      = "likely";
   const char *detail    = "";
   size_t     count      = 0;
   int        word_count = 0;
   bool       nope;
   bool       doubtful_cast = false;


   LOG_FMT(LCASTS, "%s(%d): start->text() is '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col);

   prev = chunk_get_prev_ncnlni(start);   // Issue #2279

   if (prev == nullptr)
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
   pc    = chunk_get_next_ncnl(start);
   first = pc;

   while (  pc != nullptr
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
               && language_is_set(LANG_OC | LANG_JAVA))
            || (  (  chunk_is_token(pc, CT_QUESTION)
                  || chunk_is_token(pc, CT_COMMA)
                  || chunk_is_token(pc, CT_MEMBER))
               && language_is_set(LANG_JAVA))
            || chunk_is_token(pc, CT_AMP)))
   {
      LOG_FMT(LCASTS, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));

      if (chunk_is_token(pc, CT_WORD) || (chunk_is_token(last, CT_ANGLE_CLOSE) && chunk_is_token(pc, CT_DC_MEMBER)))
      {
         word_count++;
      }
      else if (chunk_is_token(pc, CT_DC_MEMBER) || chunk_is_token(pc, CT_MEMBER) || chunk_is_token(pc, CT_PP))
      {
         // might be negativ, such as with:
         // a = val + (CFoo::bar_t)7;
         word_count--;
      }
      last = pc;
      pc   = chunk_get_next_ncnl(pc);
      count++;
   }

   if (  pc == nullptr
      || pc->type != CT_PAREN_CLOSE
      || chunk_is_token(prev, CT_OC_CLASS))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast, hit type is %s\n",
              __func__, __LINE__, pc == nullptr ? "NULL" : get_token_name(pc->type));
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
      || (chunk_is_token(last, CT_ANGLE_CLOSE) && language_is_set(LANG_OC | LANG_JAVA)))
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

      if (  (last->len() > 3)
         && (last->str[last->len() - 2] == '_')
         && (last->str[last->len() - 1] == 't'))
      {
         detail = " -- '_t'";
      }
      else if (is_ucase_str(last->text(), last->len()))
      {
         detail = " -- upper case";
      }
      else if (language_is_set(LANG_OC) && chunk_is_str(last, "id", 2))
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
      pc    = chunk_get_next_ncnl(paren_close);
      after = pc;

      do
      {
         after = chunk_get_next_ncnl(after);
      } while (chunk_is_token(after, CT_PAREN_OPEN));

      if (after == nullptr)
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - hit NULL\n",
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
         if (chunk_is_token(after, CT_STRING) || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (chunk_is_token(pc, CT_PLUS))
      {
         // (UINT8)+1 or (foo)+1
         if (  (after->type != CT_NUMBER && after->type != CT_NUMBER_FP)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (  pc->type != CT_NUMBER_FP
              && pc->type != CT_NUMBER
              && pc->type != CT_WORD
              && pc->type != CT_THIS
              && pc->type != CT_TYPE
              && pc->type != CT_PAREN_OPEN
              && pc->type != CT_STRING
              && pc->type != CT_DECLTYPE
              && pc->type != CT_SIZEOF
              && get_chunk_parent_type(pc) != CT_SIZEOF
              && pc->type != CT_FUNC_CALL
              && pc->type != CT_FUNC_CALL_USER
              && pc->type != CT_FUNCTION
              && pc->type != CT_BRACE_OPEN
              && (!(  chunk_is_token(pc, CT_SQUARE_OPEN)
                   && language_is_set(LANG_OC))))
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by text() '%s', type is %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type));
         return;
      }

      if (nope)
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - text() '%s' followed by type %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(after->type));
         return;
      }
   }
   // if the 'cast' is followed by a semicolon, comma, bool or close parenthesis, it isn't
   pc = chunk_get_next_ncnl(paren_close);

   if (pc == nullptr)
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
        pc != nullptr && pc != paren_close;
        pc = chunk_get_next_ncnl(pc))
   {
      set_chunk_parent(pc, CT_C_CAST);
      make_type(pc);
      LOG_FMT(LCASTS, " %s", pc->text());
   }

   LOG_FMT(LCASTS, " )%s\n", detail);

   // Mark the next item as an expression start
   pc = chunk_get_next_ncnl(paren_close);

   if (pc != nullptr)
   {
      chunk_flags_set(pc, PCF_EXPR_START);

      if (chunk_is_opening_brace(pc))
      {
         set_paren_parent(pc, get_chunk_parent_type(start));
      }
   }
} // fix_casts


void fix_enum_struct_union(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t     *next;
   chunk_t     *prev        = nullptr;
   pcf_flags_t flags        = PCF_VAR_1ST_DEF;
   auto const  in_fcn_paren = pc->flags & PCF_IN_FCN_DEF;

   // Make sure this wasn't a cast
   if (get_chunk_parent_type(pc) == CT_C_CAST)
   {
      return;
   }
   // the next item is either a type or open brace
   next = chunk_get_next_ncnl(pc);

   // the enum-key might be enum, enum class or enum struct (TODO)
   if (chunk_is_token(next, CT_ENUM_CLASS))
   {
      next = chunk_get_next_ncnl(next); // get the next one
   }

   if (language_is_set(LANG_CPP))
   {
      next = skip_attribute_next(next); // get the next one
   }

   // the next item is either a type, an attribute (TODO), an identifier, a colon or open brace
   if (chunk_is_token(next, CT_TYPE) || chunk_is_token(next, CT_WORD) || chunk_is_token(next, CT_COLON))
   {
      // i.e. "enum xyz : unsigned int { ... };"
      // i.e. "enum class xyz : unsigned int { ... };"
      // i.e. "enum : unsigned int { ... };"
      // xyz is a type

      // save the type if it exists
      if (!chunk_is_token(next, CT_COLON))
      {
         set_chunk_parent(next, pc->type);
         prev = next;
         next = chunk_get_next_ncnl(next);
      }

      if (next == nullptr)
      {
         return;
      }
      set_chunk_parent(next, pc->type);
      auto const is_struct_or_class =
         (chunk_is_token(pc, CT_STRUCT) || chunk_is_token(pc, CT_CLASS));

      // next up is either a colon, open brace, or open parenthesis (pawn)
      if (language_is_set(LANG_PAWN) && chunk_is_token(next, CT_PAREN_OPEN))
      {
         next = set_paren_parent(next, CT_ENUM);
      }
      else if (chunk_is_token(next, CT_COLON))
      {
         if (chunk_is_token(pc, CT_ENUM))
         {
            // enum TYPE : INT_TYPE { ... };
            next = chunk_get_next_ncnl(next);

            if (next != nullptr)
            {
               make_type(next);
               next = chunk_get_next_ncnl(next);

               // enum TYPE : unsigned int { ... };
               if (chunk_is_token(next, CT_TYPE))
               {
                  // get the next part of the type
                  next = chunk_get_next_ncnl(next);
               }
            }
         }
         else if (is_struct_or_class)
         {
            next = skip_parent_types(next);
         }
      }
      else if (is_struct_or_class && chunk_is_token(next, CT_PAREN_OPEN))
      {
         // Fix #1267 structure attributes
         // struct __attribute__(align(x)) struct_name;
         // skip to matching parenclose and make next token as type.
         next = chunk_skip_to_match(next);
         next = chunk_get_next_ncnl(next);
         set_chunk_type(next, CT_TYPE);
         set_chunk_parent(next, pc->type);
      }

      if (chunk_is_token(next, CT_SEMICOLON)) // c++ forward declaration
      {
         set_chunk_parent(next, pc->type);
         flag_series(pc, prev, PCF_INCOMPLETE);
         return;
      }
   }

   if (chunk_is_token(next, CT_BRACE_OPEN))
   {
      auto const flag = [pc] {
         switch (pc->type)
         {
         case CT_ENUM:
            return(PCF_IN_ENUM);

         case CT_STRUCT:
            return(PCF_IN_STRUCT);

         case CT_CLASS:
            return(PCF_IN_CLASS);

         default:
            return(PCF_NONE);
         }
      }();

      flag_parens(next, flag, CT_NONE, CT_NONE, false);

      if (  chunk_is_token(pc, CT_UNION)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_CLASS))
      {
         mark_struct_union_body(next);
      }
      // Skip to the closing brace
      set_chunk_parent(next, pc->type);
      next   = chunk_get_next_type(next, CT_BRACE_CLOSE, pc->level);
      flags |= PCF_VAR_INLINE;

      if (next != nullptr)
      {
         set_chunk_parent(next, pc->type);
         next = chunk_get_next_ncnl(next);
      }
      prev = nullptr;
   }
   // reset var name parent type
   else if (next && prev)
   {
      set_chunk_parent(prev, CT_NONE);
   }

   if (next == nullptr || chunk_is_token(next, CT_PAREN_CLOSE))
   {
      return;
   }

   if (!chunk_is_semicolon(next))
   {
      // Pawn does not require a semicolon after an enum
      if (language_is_set(LANG_PAWN))
      {
         return;
      }

      /*
       * D does not require a semicolon after an enum, but we add one to make
       * other code happy.
       */
      if (language_is_set(LANG_D))
      {
         next = pawn_add_vsemi_after(chunk_get_prev_ncnlni(next));   // Issue #2279
      }
   }

   // We are either pointing to a ';' or a variable
   while (  next != nullptr
         && !chunk_is_semicolon(next)
         && next->type != CT_ASSIGN
         && !(in_fcn_paren ^ (next->flags & PCF_IN_FCN_DEF)).test_any())
   {
      if (next->level == pc->level)
      {
         if (chunk_is_token(next, CT_WORD))
         {
            chunk_flags_set(next, flags);
            flags &= ~PCF_VAR_1ST;   // clear the first flag for the next items
            LOG_FMT(LCASTS, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', set PCF_VAR_1ST\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         }

         if (  chunk_is_token(next, CT_STAR)
            || (language_is_set(LANG_CPP) && chunk_is_token(next, CT_CARET)))
         {
            set_chunk_type(next, CT_PTR_TYPE);
         }

         // If we hit a comma in a function param, we are done
         if (  (chunk_is_token(next, CT_COMMA) || chunk_is_token(next, CT_FPAREN_CLOSE))
            && (next->flags.test_any(PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)))
         {
            return;
         }
      }
      next = chunk_get_next_ncnl(next);
   }

   if (  next != nullptr
      && chunk_is_token(next, CT_SEMICOLON))
   {
      set_chunk_parent(next, pc->type);
   }
} // fix_enum_struct_union


void fix_fcn_def_params(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   LOG_FMT(LFCNP, "%s(%d): text() '%s', type is %s, on orig_line %zu, level is %zu\n",
           __func__, __LINE__, start->text(), get_token_name(start->type), start->orig_line, start->level);

   while (start != nullptr && !chunk_is_paren_open(start))
   {
      start = chunk_get_next_ncnl(start);
   }

   if (start == nullptr)// Coverity CID 76003, 1100782
   {
      return;
   }
   // ensure start chunk holds a single '(' character
   assert((start->len() == 1) && (start->str[0] == '('));

   ChunkStack cs;
   size_t     level = start->level + 1;
   chunk_t    *pc   = start;

   while ((pc = chunk_get_next_ncnl(pc)) != nullptr)
   {
      if (  ((start->len() == 1) && (start->str[0] == ')'))
         || pc->level < level)
      {
         LOG_FMT(LFCNP, "%s(%d): bailed on text() '%s', on orig_line %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line);
         break;
      }
      LOG_FMT(LFCNP, "%s(%d): %s, text() '%s' on orig_line %zu, level %zu\n",
              __func__, __LINE__, (pc->level > level) ? "skipping" : "looking at",
              pc->text(), pc->orig_line, pc->level);

      if (pc->level > level)
      {
         continue;
      }

      if (chunk_is_star(pc) || chunk_is_msref(pc) || chunk_is_nullable(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
         cs.Push_Back(pc);
      }
      else if (  chunk_is_token(pc, CT_AMP)
              || (language_is_set(LANG_CPP) && chunk_is_str(pc, "&&", 2)))
      {
         set_chunk_type(pc, CT_BYREF);
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_TYPE_WRAP))
      {
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_TYPE))
      {
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_COMMA) || chunk_is_token(pc, CT_ASSIGN))
      {
         mark_variable_stack(cs, LFCNP);

         if (chunk_is_token(pc, CT_ASSIGN))
         {
            // Mark assignment for default param spacing
            set_chunk_parent(pc, CT_FUNC_PROTO);
         }
      }
   }
   mark_variable_stack(cs, LFCNP);
} // fix_fcn_def_params


void fix_type_cast(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   pc = chunk_get_next_ncnl(start);

   if (pc == nullptr || pc->type != CT_ANGLE_OPEN)
   {
      return;
   }

   while (  ((pc = chunk_get_next_ncnl(pc)) != nullptr)
         && pc->level >= start->level)
   {
      if (pc->level == start->level && chunk_is_token(pc, CT_ANGLE_CLOSE))
      {
         pc = chunk_get_next_ncnl(pc);

         if (pc == nullptr)
         {
            return;
         }

         if (chunk_is_str(pc, "(", 1))
         {
            set_paren_parent(pc, CT_TYPE_CAST);
         }
         return;
      }
      make_type(pc);
   }
} // fix_type_cast


void fix_typedef(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   LOG_FMT(LTYPEDEF, "%s(%d): typedef @ orig_line %zu, orig_col %zu\n",
           __func__, __LINE__, start->orig_line, start->orig_col);

   chunk_t *the_type = nullptr;
   chunk_t *last_op  = nullptr;

   /*
    * Mark everything in the typedef and scan for ")(", which makes it a
    * function type
    */
   for (chunk_t *next = chunk_get_next_ncnl(start, scope_e::PREPROC)
        ; next != nullptr && next->level >= start->level
        ; next = chunk_get_next_ncnl(next, scope_e::PREPROC))
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

         if (language_is_set(LANG_D) && chunk_is_token(next, CT_ASSIGN))
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
   if (  last_op != nullptr
      && !(language_is_set(LANG_OC) && get_chunk_parent_type(last_op) == CT_ENUM))
   {
      flag_parens(last_op, PCF_NONE, CT_FPAREN_OPEN, CT_TYPEDEF, false);
      fix_fcn_def_params(last_op);

      the_type = chunk_get_prev_ncnlni(last_op, scope_e::PREPROC);   // Issue #2279

      if (the_type == nullptr)
      {
         return;
      }
      chunk_t *open_paren = nullptr;

      if (chunk_is_paren_close(the_type))
      {
         open_paren = chunk_skip_to_match_rev(the_type);
         mark_function_type(the_type);
         the_type = chunk_get_prev_ncnlni(the_type, scope_e::PREPROC);   // Issue #2279

         if (the_type == nullptr)
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

      LOG_FMT(LTYPEDEF, "%s(%d): fcn typedef text() '%s', on orig_line %zu\n",
              __func__, __LINE__, the_type->text(), the_type->orig_line);

      // If we are aligning on the open parenthesis, grab that instead
      log_rule_B("align_typedef_func");

      if (open_paren != nullptr && options::align_typedef_func() == 1)
      {
         the_type = open_paren;
      }
      log_rule_B("align_typedef_func");

      if (options::align_typedef_func() != 0)
      {
         LOG_FMT(LTYPEDEF, "%s(%d):  -- align anchor on text() %s, @ orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, the_type->text(), the_type->orig_line, the_type->orig_col);
         chunk_flags_set(the_type, PCF_ANCHOR);
      }
      // already did everything we need to do
      return;
   }
   /*
    * Skip over enum/struct/union stuff, as we know it isn't a return type
    * for a function type
    */
   chunk_t *after = chunk_get_next_ncnl(start, scope_e::PREPROC);

   if (after == nullptr)
   {
      return;
   }

   if (  after->type != CT_ENUM
      && after->type != CT_STRUCT
      && after->type != CT_UNION)
   {
      if (the_type != nullptr)
      {
         // We have just a regular typedef
         LOG_FMT(LTYPEDEF, "%s(%d): regular typedef text() %s, on orig_line %zu\n",
                 __func__, __LINE__, the_type->text(), the_type->orig_line);
         chunk_flags_set(the_type, PCF_ANCHOR);
      }
      return;
   }
   // We have a struct/union/enum, next should be either a type or {
   chunk_t *next = chunk_get_next_ncnl(after, scope_e::PREPROC);

   if (next == nullptr)
   {
      return;
   }

   if (chunk_is_token(next, CT_TYPE))
   {
      next = chunk_get_next_ncnl(next, scope_e::PREPROC);

      if (next == nullptr)
      {
         return;
      }
   }

   if (chunk_is_token(next, CT_BRACE_OPEN))
   {
      // Skip to the closing brace
      chunk_t *br_c = chunk_get_next_type(next, CT_BRACE_CLOSE, next->level, scope_e::PREPROC);

      if (br_c != nullptr)
      {
         const c_token_t tag = after->type;
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

   if (the_type != nullptr)
   {
      LOG_FMT(LTYPEDEF, "%s(%d): %s typedef text() %s, on orig_line %zu\n",
              __func__, __LINE__, get_token_name(after->type), the_type->text(),
              the_type->orig_line);
      chunk_flags_set(the_type, PCF_ANCHOR);
   }
} // fix_typedef


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
} // mark_exec_sql


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

            if (  chunk_is_paren_open(tmp)
               && !pc->flags.test(PCF_IN_PREPROC))               // Issue #2703
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
} // mark_lvalue


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
} // mark_where_chunk
