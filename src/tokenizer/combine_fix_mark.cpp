/**
 * @file combine_fix_mark.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "tokenizer/combine_fix_mark.h"

#include "ChunkStack.h"
#include "log_rules.h"
#include "tokenizer/combine_skip.h"
#include "tokenizer/combine_tools.h"
#include "tokenizer/flag_parens.h"


constexpr static auto LCURRENT = LCOMBINE;


using namespace uncrustify;


void fix_casts(Chunk *start)
{
   LOG_FUNC_ENTRY();
   Chunk      *pc;
   Chunk      *prev;
   Chunk      *first;
   Chunk      *after;
   Chunk      *last = Chunk::NullChunkPtr;
   Chunk      *paren_close;
   const char *verb      = "likely";
   const char *detail    = "";
   size_t     count      = 0;
   int        word_count = 0;
   bool       nope;
   bool       doubtful_cast = false;


   LOG_FMT(LCASTS, "%s(%d): start->Text() is '%s', orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, start->Text(), start->GetOrigLine(), start->GetOrigCol());

   prev = start->GetPrevNcNnlNi();   // Issue #2279

   if (prev->IsNullChunk())
   {
      return;
   }

   if (prev->Is(CT_PP_DEFINED))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - after defined\n",
              __func__, __LINE__);
      return;
   }

   if (prev->Is(CT_ANGLE_CLOSE))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - after > (template)\n",
              __func__, __LINE__);
      return;
   }
   // Make sure there is only WORD, TYPE, and '*' or '^' before the close paren
   pc    = start->GetNextNcNnl();
   first = pc;

   while (  pc->IsNotNullChunk()
         && (  pc->IsTypeDefinition()
            || pc->Is(CT_WORD)
            || pc->Is(CT_QUALIFIER)
            || pc->Is(CT_DC_MEMBER)
            || pc->Is(CT_PP)
            || pc->Is(CT_STAR)
            || pc->Is(CT_QUESTION)
            || pc->Is(CT_CARET)
            || pc->Is(CT_TSQUARE)
            || (  (  pc->Is(CT_ANGLE_OPEN)
                  || pc->Is(CT_ANGLE_CLOSE))
               && (  language_is_set(lang_flag_e::LANG_OC)
                  || language_is_set(lang_flag_e::LANG_JAVA)
                  || language_is_set(lang_flag_e::LANG_CS)
                  || language_is_set(lang_flag_e::LANG_VALA)
                  || language_is_set(lang_flag_e::LANG_CPP)))
            || (  (  pc->Is(CT_QUESTION)
                  || pc->Is(CT_COMMA)
                  || pc->Is(CT_MEMBER))
               && (  language_is_set(lang_flag_e::LANG_JAVA)
                  || language_is_set(lang_flag_e::LANG_CS)
                  || language_is_set(lang_flag_e::LANG_VALA)))
            || (  pc->Is(CT_COMMA)
               && language_is_set(lang_flag_e::LANG_CPP))
            || pc->Is(CT_AMP)))
   {
      LOG_FMT(LCASTS, "%s(%d): pc->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));

      if (  pc->Is(CT_WORD)
         || (  last->Is(CT_ANGLE_CLOSE)
            && pc->Is(CT_DC_MEMBER)))
      {
         word_count++;
      }
      else if (  pc->Is(CT_DC_MEMBER)
              || pc->Is(CT_MEMBER)
              || pc->Is(CT_PP))
      {
         // might be negative, such as with:
         // a = val + (CFoo::bar_t)7;
         word_count--;
      }
      last = pc;
      pc   = pc->GetNextNcNnl();
      count++;
   }

   if (  pc->IsNullChunk()
      || pc->IsNot(CT_PAREN_CLOSE)
      || prev->Is(CT_OC_CLASS))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast, hit type is %s\n",
              __func__, __LINE__, pc->IsNullChunk() ? "Null chunk" : get_token_name(pc->GetType()));
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
   if (  last->Is(CT_STAR)
      || last->Is(CT_CARET)
      || last->Is(CT_PTR_TYPE)
      || last->Is(CT_TYPE)
      || (  last->Is(CT_ANGLE_CLOSE)
         && (  language_is_set(lang_flag_e::LANG_OC)
            || language_is_set(lang_flag_e::LANG_JAVA)
            || language_is_set(lang_flag_e::LANG_CS)
            || language_is_set(lang_flag_e::LANG_VALA)
            || language_is_set(lang_flag_e::LANG_CPP))))
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
         && (last->GetStr()[last->Len() - 2] == '_')
         && (last->GetStr()[last->Len() - 1] == 't'))
      {
         detail = " -- '_t'";
      }
      else if (is_ucase_str(last->Text(), last->Len()))
      {
         detail = " -- upper case";
      }
      else if (  language_is_set(lang_flag_e::LANG_OC)
              && last->IsString("id"))
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
      } while (after->Is(CT_PAREN_OPEN));

      if (after->IsNullChunk())
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - hit null chunk\n",
                 __func__, __LINE__);
         return;
      }
      nope = false;

      if (pc->IsPointerOperator())
      {
         // star (*) and address (&) are ambiguous
         if (  after->Is(CT_NUMBER_FP)
            || after->Is(CT_NUMBER)
            || after->Is(CT_STRING)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (pc->Is(CT_MINUS))
      {
         // (UINT8)-1 or (foo)-1 or (FOO)-'a'
         if (  after->Is(CT_STRING)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (pc->Is(CT_PLUS))
      {
         // (UINT8)+1 or (foo)+1
         if (  (  after->IsNot(CT_NUMBER)
               && after->IsNot(CT_NUMBER_FP))
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (  pc->IsNot(CT_NUMBER_FP)
              && pc->IsNot(CT_NUMBER)
              && pc->IsNot(CT_WORD)
              && pc->IsNot(CT_THIS)
              && pc->IsNot(CT_TYPE)
              && pc->IsNot(CT_PAREN_OPEN)
              && pc->IsNot(CT_STRING)
              && pc->IsNot(CT_DECLTYPE)
              && pc->IsNot(CT_SIZEOF)
              && pc->GetParentType() != CT_SIZEOF
              && pc->IsNot(CT_FUNC_CALL)
              && pc->IsNot(CT_FUNC_CALL_USER)
              && pc->IsNot(CT_FUNCTION)
              && pc->IsNot(CT_BRACE_OPEN)
              && (!(  pc->Is(CT_SQUARE_OPEN)
                   && language_is_set(lang_flag_e::LANG_OC))))
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()));
         return;
      }

      if (nope)
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - Text() '%s' followed by type %s\n",
                 __func__, __LINE__, pc->Text(), get_token_name(after->GetType()));
         return;
      }
   }
   // if the 'cast' is followed by a semicolon, comma, bool or close parenthesis, it isn't
   pc = paren_close->GetNextNcNnl();

   if (pc->IsNullChunk())
   {
      return;
   }

   if (  pc->IsSemicolon()
      || pc->Is(CT_COMMA)
      || pc->Is(CT_BOOL)               // Issue #2151
      || pc->IsParenClose())
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by type %s\n",
              __func__, __LINE__, get_token_name(pc->GetType()));
      return;
   }
   start->SetParentType(CT_C_CAST);
   paren_close->SetParentType(CT_C_CAST);

   LOG_FMT(LCASTS, "%s(%d):  -- %s c-cast: (",
           __func__, __LINE__, verb);

   for (pc = first;
        pc->IsNotNullChunk() && pc != paren_close;
        pc = pc->GetNextNcNnl())
   {
      pc->SetParentType(CT_C_CAST);
      make_type(pc);
      LOG_FMT(LCASTS, " %s", pc->Text());
   }

   LOG_FMT(LCASTS, " )%s\n", detail);

   // Mark the next item as an expression start
   pc = paren_close->GetNextNcNnl();

   if (pc->IsNotNullChunk())
   {
      pc->SetFlagBits(PCF_EXPR_START);

      if (pc->IsBraceOpen())
      {
         set_paren_parent(pc, start->GetParentType());
      }
   }
} // fix_casts


void fix_fcn_def_params(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start->IsNullChunk())
   {
      return;
   }
   LOG_FMT(LFCNP, "%s(%d): Text() '%s', type is %s, on orig line %zu, level is %zu\n",
           __func__, __LINE__, start->Text(), get_token_name(start->GetType()), start->GetOrigLine(), start->GetLevel());

   while (  start->IsNotNullChunk()
         && !start->IsParenOpen())
   {
      start = start->GetNextNcNnl();
   }

   if (start->IsNullChunk()) // Coverity CID 76003, 1100782
   {
      return;
   }
   // ensure start chunk holds a single '(' character
   assert(  (start->Len() == 1)
         && (start->GetStr()[0] == '('));

   ChunkStack cs;
   size_t     level = start->GetLevel() + 1;
   Chunk      *pc   = start->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (  (  (start->Len() == 1)
            && (start->GetStr()[0] == ')'))
         || pc->GetLevel() < level)
      {
         LOG_FMT(LFCNP, "%s(%d): bailed on Text() '%s', on orig line %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine());
         break;
      }
      LOG_FMT(LFCNP, "%s(%d): %s, Text() '%s' on orig line %zu, level %zu\n",
              __func__, __LINE__, (pc->GetLevel() > level) ? "skipping" : "looking at",
              pc->Text(), pc->GetOrigLine(), pc->GetLevel());

      if (pc->GetLevel() > level)
      {
         pc = pc->GetNextNcNnl();
         continue;
      }

      if (  pc->IsStar()
         || pc->IsMsRef()
         || pc->IsNullable())
      {
         pc->SetType(CT_PTR_TYPE);
         cs.Push_Back(pc);
      }
      else if (  language_is_set(lang_flag_e::LANG_CPP)   // Issue #3662
              && (  pc->Is(CT_AMP)
                 || pc->IsString("&&")))
      {
         pc->SetType(CT_BYREF);
         cs.Push_Back(pc);
      }
      else if (pc->Is(CT_TYPE_WRAP))
      {
         cs.Push_Back(pc);
      }
      else if (  pc->Is(CT_WORD)
              || pc->Is(CT_TYPE))
      {
         cs.Push_Back(pc);
      }
      else if (  pc->Is(CT_COMMA)
              || pc->Is(CT_ASSIGN))
      {
         mark_variable_stack(cs, LFCNP);

         if (pc->Is(CT_ASSIGN))
         {
            // Mark assignment for default param spacing
            pc->SetParentType(CT_FUNC_PROTO);
         }
      }
      pc = pc->GetNextNcNnl();
   }
   mark_variable_stack(cs, LFCNP);
} // fix_fcn_def_params


void fix_type_cast(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start->IsNullChunk())
   {
      return;
   }
   Chunk *pc = start->GetNextNcNnl();

   if (  pc->IsNullChunk()
      || pc->IsNot(CT_ANGLE_OPEN))
   {
      return;
   }
   pc = pc->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= start->GetLevel())
   {
      if (  pc->GetLevel() == start->GetLevel()
         && pc->Is(CT_ANGLE_CLOSE))
      {
         pc = pc->GetNextNcNnl();

         if (pc->IsNullChunk())
         {
            return;
         }

         if (pc->IsString("("))
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

   if (start->IsNullChunk())
   {
      return;
   }
   LOG_FMT(LTYPEDEF, "%s(%d): typedef @ orig line %zu, orig col %zu\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol());

   Chunk *the_type = Chunk::NullChunkPtr;
   Chunk *last_op  = Chunk::NullChunkPtr;

   /*
    * Mark everything in the typedef and scan for ")(", which makes it a
    * function type
    */
   for (Chunk *next = start->GetNextNcNnl(E_Scope::PREPROC)
        ; next->IsNotNullChunk() && next->GetLevel() >= start->GetLevel()
        ; next = next->GetNextNcNnl(E_Scope::PREPROC))
   {
      next->SetFlagBits(PCF_IN_TYPEDEF);

      if (start->GetLevel() == next->GetLevel())
      {
         if (next->IsSemicolon())
         {
            next->SetParentType(CT_TYPEDEF);
            break;
         }

         if (next->Is(CT_ATTRIBUTE))
         {
            break;
         }

         if (  language_is_set(lang_flag_e::LANG_D)
            && next->Is(CT_ASSIGN))
         {
            next->SetParentType(CT_TYPEDEF);
            break;
         }
         make_type(next);

         if (next->Is(CT_TYPE))
         {
            the_type = next;
         }
         next->ResetFlagBits(PCF_VAR_1ST_DEF);

         if (next->Is(CT_PAREN_OPEN))
         {
            last_op = next;
         }
      }
   }

   // avoid interpreting typedef NS_ENUM (NSInteger, MyEnum) as a function def
   if (  last_op->IsNotNullChunk()
      && !(  language_is_set(lang_flag_e::LANG_OC)
          && last_op->GetParentType() == CT_ENUM))
   {
      flag_parens(last_op, PCF_NONE, CT_FPAREN_OPEN, CT_TYPEDEF, false);
      fix_fcn_def_params(last_op);

      the_type = last_op->GetPrevNcNnlNi(E_Scope::PREPROC);   // Issue #2279

      if (the_type->IsNullChunk())
      {
         return;
      }
      Chunk *open_paren = Chunk::NullChunkPtr;

      if (the_type->IsParenClose())
      {
         open_paren = the_type->GetOpeningParen();
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
         the_type->SetType(CT_FUNC_TYPE);
      }
      the_type->SetParentType(CT_TYPEDEF);

      LOG_FMT(LTYPEDEF, "%s(%d): fcn typedef Text() '%s', on orig line %zu\n",
              __func__, __LINE__, the_type->Text(), the_type->GetOrigLine());

      // If we are aligning on the open parenthesis, grab that instead
      log_rule_B("align_typedef_func");

      if (  open_paren->IsNotNullChunk()
         && options::align_typedef_func() == 1)
      {
         the_type = open_paren;
      }
      log_rule_B("align_typedef_func");

      if (options::align_typedef_func() != 0)
      {
         LOG_FMT(LTYPEDEF, "%s(%d):  -- align anchor on Text() %s, @ orig line %zu, orig col %zu\n",
                 __func__, __LINE__, the_type->Text(), the_type->GetOrigLine(), the_type->GetOrigCol());
         the_type->SetFlagBits(PCF_ANCHOR);
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

   if (  after->IsNot(CT_ENUM)
      && after->IsNot(CT_STRUCT)
      && after->IsNot(CT_UNION))
   {
      if (the_type->IsNotNullChunk())
      {
         // We have just a regular typedef
         LOG_FMT(LTYPEDEF, "%s(%d): regular typedef Text() %s, on orig line %zu\n",
                 __func__, __LINE__, the_type->Text(), the_type->GetOrigLine());
         the_type->SetFlagBits(PCF_ANCHOR);
      }
      return;
   }
   // We have a struct/union/enum, next should be either a type or {
   Chunk *next = after->GetNextNcNnl(E_Scope::PREPROC);

   if (next->IsNullChunk())
   {
      return;
   }

   if (next->Is(CT_TYPE))
   {
      next = next->GetNextNcNnl(E_Scope::PREPROC);

      if (next->IsNullChunk())
      {
         return;
      }
   }

   if (next->Is(CT_BRACE_OPEN))
   {
      // Skip to the closing brace
      Chunk *br_c = next->GetNextType(CT_BRACE_CLOSE, next->GetLevel(), E_Scope::PREPROC);

      if (br_c->IsNotNullChunk())
      {
         const E_Token tag = after->GetType();
         next->SetParentType(tag);
         br_c->SetParentType(tag);

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

   if (the_type->IsNotNullChunk())
   {
      LOG_FMT(LTYPEDEF, "%s(%d): %s typedef Text() %s, on orig line %zu\n",
              __func__, __LINE__, get_token_name(after->GetType()), the_type->Text(),
              the_type->GetOrigLine());
      the_type->SetFlagBits(PCF_ANCHOR);
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

   LOG_FMT(LFVD, "%s(%d): start at pc orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
   log_pcf_flags(LFCNR, pc->GetFlags());

   // Scan for words and types and stars oh my!
   while (  pc->Is(CT_TYPE)
         || pc->Is(CT_WORD)
         || pc->Is(CT_QUALIFIER)
         || pc->Is(CT_TYPENAME)
         || pc->Is(CT_DC_MEMBER)
         || pc->Is(CT_MEMBER)
         || pc->Is(CT_PP)                       // Issue #3169
         || pc->IsPointerOperator())
   {
      LOG_FMT(LFVD, "%s(%d):   1:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()));
      cs.Push_Back(pc);
      pc = pc->GetNextNcNnl();

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFVD, "%s(%d): pc is null chunk\n", __func__, __LINE__);
         return(Chunk::NullChunkPtr);
      }
      LOG_FMT(LFVD, "%s(%d):   2:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()));

      // Skip templates and attributes
      pc = skip_template_next(pc);

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFVD, "%s(%d): pc is null chunk\n", __func__, __LINE__);
         return(Chunk::NullChunkPtr);
      }
      LOG_FMT(LFVD, "%s(%d):   3:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()));

      pc = skip_attribute_next(pc);

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFVD, "%s(%d): pc is null chunk\n", __func__, __LINE__);
         return(Chunk::NullChunkPtr);
      }
      LOG_FMT(LFVD, "%s(%d):   4:pc->Text() '%s', type is %s\n",
              __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()));

      if (language_is_set(lang_flag_e::LANG_JAVA))
      {
         pc = skip_tsquare_next(pc);

         if (pc->IsNotNullChunk())
         {
            LOG_FMT(LFVD, "%s(%d):   5:pc->Text() '%s', type is %s\n", __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()));
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
      return(Chunk::NullChunkPtr);
   }
   LOG_FMT(LFVD, "%s(%d): end->GetType() is %s\n", __func__, __LINE__, get_token_name(end->GetType()));

   if (end->Is(CT_FUNC_CTOR_VAR))                // Issue #3010
   {
      return(end);
   }

   if (  cs.Len() == 1
      && end->Is(CT_BRACE_OPEN)
      && end->GetParentType() == CT_BRACED_INIT_LIST)
   {
      cs.Get(0)->m_pc->SetType(CT_TYPE);
   }

   // Function defs are handled elsewhere
   if (  (cs.Len() <= 1)
      || end->Is(CT_FUNC_DEF)
      || end->Is(CT_FUNC_PROTO)
      || end->Is(CT_FUNC_CLASS_DEF)
      || end->Is(CT_FUNC_CLASS_PROTO)
      || end->Is(CT_OPERATOR))
   {
      return(skip_to_next_statement(end));
   }
   // ref_idx points to the alignable part of the variable definition
   ref_idx = cs.Len() - 1;

   // Check for the '::' stuff: "char *Engine::name"
   if (  (cs.Len() >= 3)
      && (  (cs.Get(cs.Len() - 2)->m_pc->GetType() == CT_MEMBER)
         || (cs.Get(cs.Len() - 2)->m_pc->GetType() == CT_DC_MEMBER)))
   {
      idx = cs.Len() - 2;

      while (idx > 0)
      {
         tmp_pc = cs.Get(idx)->m_pc;

         if (  tmp_pc->IsNot(CT_DC_MEMBER)
            && tmp_pc->IsNot(CT_MEMBER))
         {
            break;
         }
         idx--;
         tmp_pc = cs.Get(idx)->m_pc;

         if (  tmp_pc->IsNot(CT_WORD)
            && tmp_pc->IsNot(CT_TYPE))
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
   LOG_FMT(LFVD2, "%s(%d): orig line is %zu, TYPE : ", __func__, __LINE__, start->GetOrigLine());

   for (size_t idxForCs = 0; idxForCs < cs.Len() - 1; idxForCs++)
   {
      tmp_pc = cs.Get(idxForCs)->m_pc;
      make_type(tmp_pc);
      tmp_pc->SetFlagBits(PCF_VAR_TYPE);
      LOG_FMT(LFVD2, " Text() is '%s', type is %s", tmp_pc->Text(), get_token_name(tmp_pc->GetType()));
   }

   LOG_FMT(LFVD2, "\n");

   // OK we have two or more items, mark types up to the end.
   LOG_FMT(LFVD, "%s(%d): pc orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
   mark_variable_definition(cs.Get(cs.Len() - 1)->m_pc);

   if (end->Is(CT_COMMA))
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

   if (  tmp->Is(CT_INV)
      || tmp->Is(CT_DESTRUCTOR))
   {
      tmp->SetType(CT_DESTRUCTOR);
      pc->SetParentType(CT_DESTRUCTOR);
      is_destr = true;
   }
   LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, FOUND %sSTRUCTOR for '%s'[%s] prev '%s'[%s]\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(),
           is_destr ? "DE" : "CON",
           pc->Text(), get_token_name(pc->GetType()),
           tmp->Text(), get_token_name(tmp->GetType()));

   paren_open = skip_template_next(pc->GetNextNcNnl());

   if (!paren_open->IsString("("))
   {
      LOG_FMT(LWARN, "%s:%zu Expected '(', got: [%s]\n",
              cpd.filename.c_str(), paren_open->GetOrigLine(),
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
         && (  tmp->IsNot(CT_BRACE_OPEN)
            || tmp->GetLevel() != paren_open->GetLevel())
         && !tmp->IsSemicolon())
   {
      LOG_FMT(LFTOR, "%s(%d): tmp is '%s', orig line is %zu, orig col is %zu\n",
              __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
      tmp->SetFlagBits(PCF_IN_CONST_ARGS);

      if (tmp->Is(CT_BRACE_OPEN))
      {
         if (!tmp->TestFlags(PCF_IN_STRUCT))                              // Issue #4248
         {
            // this opens a new block,
            // look for the end of the block
            Chunk *closing = tmp->GetNextType(CT_BRACE_CLOSE, tmp->GetLevel());
            LOG_FMT(LFTOR, "%s(%d): closing is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, closing->Text(), closing->GetOrigLine(), closing->GetOrigCol());
            tmp = closing;
         }
      }
      tmp = tmp->GetNextNcNnl();

      if (  tmp->IsString(":")
         && tmp->GetLevel() == paren_open->GetLevel())
      {
         tmp->SetType(CT_CONSTR_COLON);
         hit_colon = true;
      }

      if (  hit_colon
         && (  tmp->IsParenOpen()
            || tmp->IsBraceOpen())
         && tmp->GetLevel() == paren_open->GetLevel())
      {
         var = skip_template_prev(tmp->GetPrevNcNnlNi());   // Issue #2279

         if (  var->Is(CT_TYPE)
            || var->Is(CT_WORD))
         {
            var->SetType(CT_FUNC_CTOR_VAR);
            flag_parens(tmp, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CTOR_VAR, false);
         }
      }
   }

   if (tmp->Is(CT_BRACE_OPEN))
   {
      set_paren_parent(paren_open, CT_FUNC_CLASS_DEF);
      set_paren_parent(tmp, CT_FUNC_CLASS_DEF);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_DEF on orig line %zu, orig col %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
   }
   else
   {
      tmp->SetParentType(CT_FUNC_CLASS_PROTO);
      pc->SetType(CT_FUNC_CLASS_PROTO);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_PROTO on orig line %zu, orig col %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
   }
   tmp = pc->GetPrevNcNnlNi(); // Issue #2907

   if (tmp->Is(CT_DESTRUCTOR))
   {
      tmp->SetParentType(pc->GetType());
      tmp = tmp->GetPrevNcNnlNi();
   }

   while (tmp->Is(CT_QUALIFIER))
   {
      tmp->SetParentType(pc->GetType());
      tmp = tmp->GetPrevNcNnlNi();
   }
} // mark_cpp_constructor


void mark_cpp_lambda(Chunk *square_open)
{
   if (  square_open->Is(CT_SQUARE_OPEN)
      && square_open->GetParentType() == CT_CPP_LAMBDA)
   {
      auto *brace_close = square_open->GetNextType(CT_BRACE_CLOSE, square_open->GetLevel());

      if (brace_close->GetParentType() == CT_CPP_LAMBDA)
      {
         for (auto *pc = square_open; pc != brace_close; pc = pc->GetNextNcNnl())
         {
            pc->SetFlagBits(PCF_IN_LAMBDA);
         }
      }
   }
} // mark_cpp_lambda


void mark_define_expressions()
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
         if (  pc->Is(CT_PP_DEFINE)
            || pc->Is(CT_PP_IF)
            || pc->Is(CT_PP_ELSE))
         {
            in_define = true;
            first     = true;
         }
      }
      else
      {
         if (  !pc->TestFlags(PCF_IN_PREPROC)
            || pc->Is(CT_PREPROC))
         {
            in_define = false;
         }
         else
         {
            if (  pc->IsNot(CT_MACRO)
               && (  first
                  || prev->Is(CT_PAREN_OPEN)
                  || prev->Is(CT_ARITH)
                  || prev->Is(CT_SHIFT)
                  || prev->Is(CT_CARET)
                  || prev->Is(CT_ASSIGN)
                  || prev->Is(CT_COMPARE)
                  || prev->Is(CT_RETURN)
                  || prev->Is(CT_GOTO)
                  || prev->Is(CT_CONTINUE)
                  || prev->Is(CT_FPAREN_OPEN)
                  || prev->Is(CT_SPAREN_OPEN)
                  || prev->Is(CT_BRACE_OPEN)
                  || prev->IsSemicolon()
                  || prev->Is(CT_COMMA)
                  || prev->Is(CT_COLON)
                  || prev->Is(CT_QUESTION)))
            {
               pc->SetFlagBits(PCF_EXPR_START);
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
   for (tmp = pc->GetNext(); tmp->IsNotNullChunk(); tmp = tmp->GetNext())
   {
      tmp->SetParentType(pc->GetType());

      if (tmp->Is(CT_WORD))
      {
         tmp->SetType(CT_SQL_WORD);
      }

      if (tmp->Is(CT_SEMICOLON))
      {
         break;
      }
   }

   if (  pc->IsNot(CT_SQL_BEGIN)
      || tmp->IsNullChunk()
      || tmp->IsNot(CT_SEMICOLON))
   {
      return;
   }

   for (tmp = tmp->GetNext();
        tmp->IsNotNullChunk() && tmp->IsNot(CT_SQL_END);
        tmp = tmp->GetNext())
   {
      tmp->SetLevel(tmp->GetLevel() + 1);
   }
} // mark_exec_sql


void mark_function_return_type(Chunk *fname, Chunk *start, E_Token parent_type)
{
   LOG_FUNC_ENTRY();
   Chunk *pc = start;

   if (pc->IsNotNullChunk())
   {
      // Step backwards from pc and mark the parent of the return type
      LOG_FMT(LFCNR, "%s(%d): (backwards) return type for '%s' @ orig line is %zu, orig col is %zu\n",
              __func__, __LINE__, fname->Text(), fname->GetOrigLine(), fname->GetOrigCol());

      Chunk *first = pc;

      while (pc->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s, ",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
         log_pcf_flags(LFCNR, pc->GetFlags());

         if (pc->Is(CT_ANGLE_CLOSE))
         {
            pc = skip_template_prev(pc);

            if (  pc->IsNullChunk()
               || pc->Is(CT_TEMPLATE))
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

         if (  (  !pc->IsTypeDefinition()
               && pc->IsNot(CT_OPERATOR)
               && pc->IsNot(CT_WORD)
               && pc->IsNot(CT_ADDR))
            || pc->TestFlags(PCF_IN_PREPROC))
         {
            break;
         }

         if (!pc->IsPointerOperator())
         {
            first = pc;
         }
         pc = pc->GetPrevNcNnlNi();   // Issue #2279
      }
      LOG_FMT(LFCNR, "%s(%d): marking returns...", __func__, __LINE__);

      // Changing words to types into tuple return types in CS.
      bool is_return_tuple = false;

      if (  pc->Is(CT_PAREN_CLOSE)
         && !pc->TestFlags(PCF_IN_PREPROC))
      {
         first           = pc->GetOpeningParen();
         is_return_tuple = true;
      }
      pc = first;

      while (pc->IsNotNullChunk())
      {
         LOG_CHUNK(LTOK, pc);

         if (parent_type != CT_NONE)
         {
            pc->SetParentType(parent_type);
         }
         Chunk *prev = pc->GetPrevNcNnlNi();   // Issue #2279

         if (  !is_return_tuple
            || pc->IsNot(CT_WORD)
            || (  prev->IsNullChunk()
               && prev->IsNot(CT_TYPE)))
         {
            make_type(pc);
         }

         if (pc == start)
         {
            break;
         }
         pc = pc->GetNextNcNnl();

         //template angles should keep parent type CT_TEMPLATE
         if (pc->Is(CT_ANGLE_OPEN))
         {
            pc = pc->GetNextType(CT_ANGLE_CLOSE, pc->GetLevel());

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
         && first->TestFlags(PCF_IN_CLASS))
      {
         pc = first->GetPrevNcNnlNi();   // Issue #2279

         if (pc->Is(CT_FRIEND))
         {
            LOG_FMT(LFCNR, "%s(%d): marking friend\n", __func__, __LINE__);
            pc->SetParentType(parent_type);
            // A friend might be preceded by a template specification, as in:
            //   template <...> friend type func(...);
            // If so, we need to mark that also
            pc = pc->GetPrevNcNnlNi();   // Issue #2279

            if (pc->Is(CT_ANGLE_CLOSE))
            {
               pc = skip_template_prev(pc);

               if (pc->Is(CT_TEMPLATE))
               {
                  LOG_FMT(LFCNR, "%s(%d): marking friend template\n",
                          __func__, __LINE__);
                  pc->SetParentType(parent_type);
               }
            }
         }
      }
   }
} // mark_function_return_type


void mark_function(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNullChunk())
   {
      return;
   }
   LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, text '%s'\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
   Chunk *prev = pc->GetPrevNcNnlNi();   // Issue #2279
   Chunk *next = pc->GetNextNppOrNcNnl();

   if (next->IsNullChunk())
   {
      return;
   }
   Chunk *tmp;
   Chunk *semi = Chunk::NullChunkPtr;
   Chunk *paren_open;
   Chunk *paren_close;

   // Find out what is before the operator
   if (pc->GetParentType() == CT_OPERATOR)
   {
      LOG_FMT(LFCN, "%s(%d): orig line %zu, orig col %zu, text '%s",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
      log_pcf_flags(LFCN, pc->GetFlags());
      Chunk *pc_op = pc->GetPrevType(CT_OPERATOR, pc->GetLevel());

      if (  pc_op->IsNotNullChunk()
         && pc_op->TestFlags(PCF_EXPR_START))
      {
         LOG_FMT(LFCN, "%s(%d): (4) SET TO CT_FUNC_CALL: orig line %zu, orig col %zu, text '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
         pc->SetType(CT_FUNC_CALL);
      }

      if (language_is_set(lang_flag_e::LANG_CPP))
      {
         tmp = pc;

         while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
         {
            if (  tmp->Is(CT_BRACE_CLOSE)
               || tmp->Is(CT_SEMICOLON))
            {
               break;
            }

            if (  tmp->IsParenOpen()
               && !pc->TestFlags(PCF_IN_PREPROC))               // Issue #2703
            {
               LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s'\n",
                       __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());
               LOG_FMT(LFCN, "%s(%d): (5) SET TO CT_FUNC_CALL: orig line is %zu, orig col is %zu, Text() '%s'\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
               pc->SetType(CT_FUNC_CALL);
               break;
            }

            if (tmp->Is(CT_ASSIGN))
            {
               LOG_FMT(LFCN, "%s(%d): (6) SET TO CT_FUNC_CALL: orig line is %zu, orig col is %zu, Text() '%s'\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
               pc->SetType(CT_FUNC_CALL);
               break;
            }

            if (tmp->Is(CT_TEMPLATE))
            {
               LOG_FMT(LFCN, "%s(%d): (7) SET TO CT_FUNC_DEF: orig line is %zu, orig col is %zu, Text() '%s'\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
               pc->SetType(CT_FUNC_DEF);
               break;
            }

            if (tmp->Is(CT_BRACE_OPEN))
            {
               if (tmp->GetParentType() == CT_FUNC_DEF)
               {
                  LOG_FMT(LFCN, "%s(%d): (8) SET TO CT_FUNC_CALL: orig line is %zu, orig col is %zu, Text() '%s'\n",
                          __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
                  pc->SetType(CT_FUNC_CALL);
               }

               if (  tmp->GetParentType() == CT_CLASS
                  || tmp->GetParentType() == CT_STRUCT)
               {
                  LOG_FMT(LFCN, "%s(%d): (9) SET TO CT_FUNC_DEF: orig line is %zu, orig col is %zu, Text() '%s'\n",
                          __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
                  pc->SetType(CT_FUNC_DEF);
               }
               break;
            }
         }

         if (  tmp->IsNotNullChunk()
            && pc->IsNot(CT_FUNC_CALL))
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

   if (  next->IsPointerOperator()
      || next->IsNewline())
   {
      next = next->GetNextNppOrNcNnl();

      if (next->IsNullChunk())
      {
         return;
      }
   }
   LOG_FMT(LFCN, "%s(%d): orig line %zu, orig col %zu, text '%s', type %s, parent type %s\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(),
           get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));
   LOG_FMT(LFCN, "   level %zu, brace level %zu, next->text '%s', next->type %s, next->level is %zu\n",
           pc->GetLevel(), pc->GetBraceLevel(),
           next->Text(), get_token_name(next->GetType()), next->GetLevel());

   if (pc->TestFlags(PCF_IN_CONST_ARGS))
   {
      pc->SetType(CT_FUNC_CTOR_VAR);
      LOG_FMT(LFCN, "%s(%d):   1) Marked [%s] as FUNC_CTOR_VAR on line %zu col %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      next = skip_template_next(next);

      if (next->IsNullChunk())
      {
         return;
      }
      flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, pc->GetType(), true);
      return;
   }
   // Skip over any template and attribute madness
   next = skip_template_next(next);

   if (next->IsNullChunk())
   {
      return;
   }
   next = skip_attribute_next(next);

   if (next->IsNullChunk())
   {
      return;
   }
   // Find the open and close parenthesis
   paren_open  = pc->GetNextString("(", 1, pc->GetLevel());
   paren_close = paren_open->GetNextString(")", 1, pc->GetLevel());

   if (  paren_open->IsNullChunk()
      || paren_close->IsNullChunk())
   {
      LOG_FMT(LFCN, "%s(%d): No parens found for [%s] on orig line %zu, orig col %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      return;
   }
   /*
    * This part detects either chained function calls or a function ptr definition.
    * MYTYPE (*func)(void);
    * MYTYPE (*func(param))(void);
    * MYTYPE (*func(param_call1)(param_call2))(void);
    * mWriter( "class Clst_"c )( somestr.getText() )( " : Cluster {"c ).newline;
    *
    * For it to be a function variable def, there must be a '*' followed by a
    * single word or by a sequence of one or more expressions each within brackets.
    *
    * Otherwise, it must be chained function calls.
    */
   tmp = paren_close->GetNextNcNnl();

   if (  tmp->IsNotNullChunk()
      && tmp->IsString("("))
   {
      Chunk *tmp1;
      Chunk *tmp2;
      Chunk *tmp3;

      // skip over any leading class/namespace in: "T(F::*A)();"
      tmp1 = next->GetNextNcNnl();

      while (tmp1->IsNotNullChunk())
      {
         tmp2 = tmp1->GetNextNcNnl();

         if (  !tmp1->IsWord()
            || tmp2->IsNot(CT_DC_MEMBER))
         {
            break;
         }
         tmp1 = tmp2->GetNextNcNnl();
      }
      tmp2 = tmp1->GetNextNcNnl();

      if (tmp2->IsString(")"))
      {
         tmp3 = tmp2;
         tmp2 = Chunk::NullChunkPtr;
      }
      else
      {
         tmp3 = tmp2->GetNextNcNnl();
      }
      tmp3 = tmp3->GetNextNbsb();

      // Issue #3852
      while (tmp3->IsString("("))
      {
         tmp3 = tmp3->GetClosingParen();
         tmp3 = tmp3->GetNextNcNnl();
      }

      if (  tmp3->IsString(")")
         && (  tmp1->IsStar()
            || tmp1->IsMsRef()
            || (  language_is_set(lang_flag_e::LANG_OC)
               && tmp1->Is(CT_CARET)))
         && (  tmp2->IsNullChunk()
            || tmp2->Is(CT_WORD)))
      {
         if (tmp2->IsNotNullChunk())
         {
            LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, function variable '%s', changing '%s' into a type\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), tmp2->Text(), pc->Text());
            tmp2->SetType(CT_FUNC_VAR);
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_VAR, false);

            LOG_FMT(LFCN, "%s(%d): paren open @ orig line %zu, orig col %zu\n",
                    __func__, __LINE__, paren_open->GetOrigLine(), paren_open->GetOrigCol());
         }
         else
         {
            LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, function type, changing '%s' into a type\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());

            if (tmp2->IsNotNullChunk())
            {
               tmp2->SetType(CT_FUNC_TYPE);
            }
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_TYPE, false);
         }
         pc->SetType(CT_TYPE);
         tmp1->SetType(CT_PTR_TYPE);
         pc->ResetFlagBits(PCF_VAR_1ST_DEF);

         if (tmp2->IsNotNullChunk())
         {
            tmp2->SetFlagBits(PCF_VAR_1ST_DEF);
         }
         flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_PROTO, false);
         fix_fcn_def_params(tmp);
         return;
      }
      LOG_FMT(LFCN, "%s(%d): chained function calls? Text() is '%s', orig line is %zu, orig col is %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
   }

   // Assume it is a function call if not already labeled
   if (pc->Is(CT_FUNCTION))
   {
      LOG_FMT(LFCN, "%s(%d): examine: Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      // look for an assignment. Issue #575
      Chunk *temp = pc->GetNextType(CT_ASSIGN, pc->GetLevel());

      if (temp->IsNotNullChunk())
      {
         LOG_FMT(LFCN, "%s(%d): assignment found, orig line is %zu, orig col is %zu, Text() '%s'\n",
                 __func__, __LINE__, temp->GetOrigLine(), temp->GetOrigCol(), temp->Text());
         LOG_FMT(LFCN, "%s(%d): (10) SET TO CT_FUNC_CALL: orig line is %zu, orig col is %zu, Text() '%s'",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
         pc->SetType(CT_FUNC_CALL);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): (11) SET TO %s: orig line is %zu, orig col is %zu, Text() '%s'",
                 __func__, __LINE__, (pc->GetParentType() == CT_OPERATOR) ? "CT_FUNC_DEF" : "CT_FUNC_CALL",
                 pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
         pc->SetType((pc->GetParentType() == CT_OPERATOR) ? CT_FUNC_DEF : CT_FUNC_CALL);
      }
   }
   LOG_FMT(LFCN, "%s(%d): Check for C++ function def, Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));

   if (prev->IsNotNullChunk())
   {
      LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
              __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(), get_token_name(prev->GetType()));
   }

   // Check for C++ function def
   if (  pc->Is(CT_FUNC_CLASS_DEF)
      || (  prev->IsNotNullChunk()
         && (  prev->Is(CT_INV)
            || prev->Is(CT_DC_MEMBER))))
   {
      Chunk *destr = Chunk::NullChunkPtr;

      if (prev->Is(CT_INV))
      {
         // TODO: do we care that this is the destructor?
         prev->SetType(CT_DESTRUCTOR);
         pc->SetType(CT_FUNC_CLASS_DEF);

         pc->SetParentType(CT_DESTRUCTOR);

         destr = prev;
         // Point to the item previous to the class name
         prev = prev->GetPrevNcNnlNpp();
      }

      if (prev->Is(CT_DC_MEMBER))
      {
         prev = prev->GetPrevNcNnlNpp();

         if (prev->IsNotNullChunk())
         {
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(),
                    get_token_name(prev->GetType()));
            prev = skip_template_prev(prev);
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(),
                    get_token_name(prev->GetType()));
            prev = skip_attribute_prev(prev);
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(),
                    get_token_name(prev->GetType()));
         }

         if (  prev->Is(CT_WORD)
            || prev->Is(CT_TYPE))
         {
            if (pc->GetStr().equals(prev->GetStr()))
            {
               LOG_FMT(LFCN, "%s(%d): pc->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(),
                       get_token_name(prev->GetType()));
               pc->SetType(CT_FUNC_CLASS_DEF);
               LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu - FOUND %sSTRUCTOR for '%s', type is %s\n",
                       __func__, __LINE__,
                       prev->GetOrigLine(), prev->GetOrigCol(),
                       (destr->IsNotNullChunk()) ? "DE" : "CON",
                       prev->Text(), get_token_name(prev->GetType()));

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
   if (  pc->Is(CT_FUNC_CALL)
      && (  pc->GetLevel() == pc->GetBraceLevel()
         || pc->GetLevel() == 1)
      && !pc->TestFlags(PCF_IN_ARRAY_ASSIGN))
   {
      bool isa_def  = false;
      bool hit_star = false;
      LOG_FMT(LFCN, "%s(%d): pc->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(),
              get_token_name(pc->GetType()));

      if (prev->IsNullChunk())
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev is null chunk\n",
                 __func__, __LINE__);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev->Text() '%s', prev->GetType() is %s\n",
                 __func__, __LINE__, prev->Text(), get_token_name(prev->GetType()));
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
         LOG_FMT(LFCN, "%s(%d): next step with: prev orig line is %zu, orig col is %zu, Text() '%s'\n",
                 __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());

         if (pc->GetParentType() == CT_FIXED)
         {
            isa_def = true;
         }

         if (prev->TestFlags(PCF_IN_PREPROC))
         {
            prev = prev->GetPrevNcNnlNpp();
            continue;
         }

         // Some code slips an attribute between the type and function
         if (  prev->Is(CT_FPAREN_CLOSE)
            && prev->GetParentType() == CT_ATTRIBUTE)
         {
            prev = skip_attribute_prev(prev);
            continue;
         }

         // skip const(TYPE)
         if (  prev->Is(CT_PAREN_CLOSE)
            && prev->GetParentType() == CT_D_CAST)
         {
            LOG_FMT(LFCN, "%s(%d): --> For sure a prototype or definition\n",
                    __func__, __LINE__);
            isa_def = true;
            break;
         }

         if (prev->GetParentType() == CT_DECLSPEC)  // Issue 1289
         {
            prev = prev->GetOpeningParen();

            if (prev->IsNotNullChunk())
            {
               prev = prev->GetPrev();
            }

            if (prev->Is(CT_DECLSPEC))
            {
               if (prev->IsNotNullChunk())
               {
                  prev = prev->GetPrev();
               }
            }
         }

         // if it was determined that this could be a function definition
         // but one of the preceding tokens is a CT_MEMBER than this is not a
         // fcn def, issue #1466
         if (  isa_def
            && prev->Is(CT_MEMBER))
         {
            isa_def = false;
         }

         // get first chunk before: A::B::pc | this.B.pc | this->B->pc
         if (  prev->Is(CT_DC_MEMBER)
            || prev->Is(CT_MEMBER))
         {
            while (  prev->Is(CT_DC_MEMBER)
                  || prev->Is(CT_MEMBER))
            {
               prev = prev->GetPrevNcNnlNpp();

               if (  prev->IsNullChunk()
                  || (  prev->IsNot(CT_WORD)
                     && prev->IsNot(CT_TYPE)
                     && prev->IsNot(CT_THIS)))
               {
                  LOG_FMT(LFCN, "%s(%d): --? skipped MEMBER and landed on %s\n",
                          __func__, __LINE__, (prev->IsNullChunk()) ? "<null chunk>" : get_token_name(prev->GetType()));
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
                  LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s'\n",
                          __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->Text());
               }
            }

            if (prev->IsNullChunk())
            {
               break;
            }
         }

         // If we are on a TYPE or WORD, then this could be a proto or def
         if (  prev->Is(CT_TYPE)
            || prev->Is(CT_WORD))
         {
            if (!hit_star)
            {
               LOG_FMT(LFCN, "%s(%d):   --> For sure a prototype or definition\n",
                       __func__, __LINE__);
               isa_def = true;
               break;
            }
            Chunk *prev_prev = prev->GetPrevNcNnlNpp();

            if (!prev_prev->Is(CT_QUESTION))               // Issue #1753
            {
               LOG_FMT(LFCN, "%s(%d):   --> maybe a proto/def\n",
                       __func__, __LINE__);

               LOG_FMT(LFCN, "%s(%d): prev is '%s', orig line is %zu, orig col is %zu, type is %s, parent type is %s\n",
                       __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(),
                       get_token_name(prev->GetType()), get_token_name(prev->GetParentType()));
               log_pcf_flags(LFCN, pc->GetFlags());
               isa_def = true;
            }
         }

         if (prev->IsPointerOperator())
         {
            hit_star = true;
         }

         if (  prev->IsNot(CT_OPERATOR)
            && prev->IsNot(CT_TSQUARE)
            && prev->IsNot(CT_ANGLE_CLOSE)
            && prev->IsNot(CT_QUALIFIER)
            && prev->IsNot(CT_TYPE)
            && prev->IsNot(CT_WORD)
            && !prev->IsPointerOperator())
         {
            LOG_FMT(LFCN, "%s(%d):  --> Stopping on prev is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                    __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol(), get_token_name(prev->GetType()));

            // certain tokens are unlikely to precede a prototype or definition
            if (  prev->Is(CT_ARITH)
               || prev->Is(CT_SHIFT)
               || prev->Is(CT_ASSIGN)
               || prev->Is(CT_COMMA)
               || (  prev->Is(CT_STRING)
                  && prev->GetParentType() != CT_EXTERN)  // fixes issue 1259
               || prev->Is(CT_STRING_MULTI)
               || prev->Is(CT_NUMBER)
               || prev->Is(CT_NUMBER_FP)
               || prev->Is(CT_FPAREN_OPEN)) // issue #1464
            {
               isa_def = false;
            }
            break;
         }

         // Skip over template and attribute stuff
         if (prev->Is(CT_ANGLE_CLOSE))
         {
            prev = skip_template_prev(prev);
         }
         else
         {
            prev = prev->GetPrevNcNnlNpp();
         }
      }
      //LOG_FMT(LFCN, " -- stopped on %s [%s]\n",
      //        prev->Text(), get_token_name(prev->GetType()));

      // Fixes issue #1634
      if (prev->IsParenClose())
      {
         Chunk *preproc = prev->GetNextNcNnl();

         if (preproc->Is(CT_PREPROC))
         {
            size_t pp_level = preproc->GetPpLevel();

            if (preproc->GetNextNcNnl()->Is(CT_PP_ELSE))
            {
               do
               {
                  preproc = preproc->GetPrevNcNnlNi();      // Issue #2279

                  if (preproc->Is(CT_PP_IF))
                  {
                     preproc = preproc->GetPrevNcNnlNi();   // Issue #2279

                     if (preproc->GetPpLevel() == pp_level)
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
         && prev->IsNotNullChunk()
         && (  (  prev->IsParenClose()
               && prev->GetParentType() != CT_D_CAST
               && prev->GetParentType() != CT_MACRO_OPEN  // Issue #2726
               && prev->GetParentType() != CT_MACRO_CLOSE)
            || prev->Is(CT_ASSIGN)
            || prev->Is(CT_RETURN)))
      {
         LOG_FMT(LFCN, "%s(%d): -- overriding DEF due to prev is '%s', type is %s\n",
                 __func__, __LINE__, prev->Text(), get_token_name(prev->GetType()));
         isa_def = false;
      }

      // Fixes issue #1266, identification of a tuple return type in CS.
      if (  !isa_def
         && prev->Is(CT_PAREN_CLOSE)
         && prev->GetNextNcNnl() == pc)
      {
         tmp = prev->GetOpeningParen();

         while (  tmp->IsNotNullChunk() // Issue #2315
               && tmp != prev)
         {
            if (  tmp->Is(CT_COMMA)
               && tmp->GetLevel() == prev->GetLevel() + 1)
            {
               LOG_FMT(LFCN, "%s(%d): -- overriding call due to tuple return type -- prev is '%s', type is %s\n",
                       __func__, __LINE__, prev->Text(), get_token_name(prev->GetType()));
               isa_def = true;
               break;
            }
            tmp = tmp->GetNextNcNnl();
         }
      }

      if (isa_def)
      {
         LOG_FMT(LFCN, "%s(%d): pc is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
         LOG_FMT(LFCN, "%s(%d): (12) SET TO CT_FUNC_DEF: orig line is %zu, orig col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
         pc->SetType(CT_FUNC_DEF);

         if (prev->IsNullChunk())
         {
            prev = Chunk::GetHead();
         }

         for (tmp = prev; tmp->IsNotNullChunk() && tmp != pc; tmp = tmp->GetNextNcNnlNpp())
         {
            LOG_FMT(LFCN, "%s(%d): Text() is '%s', type is %s\n",
                    __func__, __LINE__, tmp->Text(), get_token_name(tmp->GetType()));
            make_type(tmp);
         }
      }
   }

   if (pc->IsNot(CT_FUNC_DEF))
   {
      LOG_FMT(LFCN, "%s(%d):  Detected type %s, Text() is '%s', on orig line %zu, orig col %zu\n",
              __func__, __LINE__, get_token_name(pc->GetType()),
              pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

      tmp = flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);

      if (  tmp->IsNotNullChunk()
         && tmp->Is(CT_BRACE_OPEN)
         && tmp->GetParentType() != CT_DOUBLE_BRACE)
      {
         set_paren_parent(tmp, pc->GetType());
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
      if (tmp->GetLevel() < pc->GetLevel())
      {
         // No semicolon - guess that it is a prototype
         pc->ResetFlagBits(PCF_VAR_1ST_DEF);
         pc->SetType(CT_FUNC_PROTO);
         break;
      }
      else if (tmp->GetLevel() == pc->GetLevel())
      {
         if (tmp->Is(CT_BRACE_OPEN))
         {
            // its a function def for sure
            break;
         }
         else if (tmp->IsSemicolon())
         {
            // Set the parent for the semicolon for later
            semi = tmp;
            pc->ResetFlagBits(PCF_VAR_1ST_DEF);
            pc->SetType(CT_FUNC_PROTO);
            LOG_FMT(LFCN, "%s(%d):   2) Marked Text() is '%s', as FUNC_PROTO on orig line %zu, orig col %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
            break;
         }
         else if (pc->Is(CT_COMMA))
         {
            pc->SetType(CT_FUNC_CTOR_VAR);
            LOG_FMT(LFCN, "%s(%d):   2) Marked Text() is '%s', as FUNC_CTOR_VAR on orig line %zu, orig col %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
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
   if (  language_is_set(lang_flag_e::LANG_CPP)
      && pc->Is(CT_FUNC_PROTO)
      && pc->GetParentType() != CT_OPERATOR)
   {
      LOG_FMT(LFPARAM, "%s(%d):", __func__, __LINE__);
      LOG_FMT(LFPARAM, "  checking '%s' for constructor variable %s %s\n",
              pc->Text(),
              get_token_name(paren_open->GetType()),
              get_token_name(paren_close->GetType()));

      /*
       * Check the token at the start of the statement. If it's 'extern', we
       * definitely have a function prototype.
       */
      tmp = pc;

      while (  tmp->IsNotNullChunk()
            && !tmp->TestFlags(PCF_STMT_START))
      {
         tmp = tmp->GetPrevNcNnlNi();   // Issue #2279
      }
      const bool is_extern = (  tmp->IsNotNullChunk()
                             && tmp->GetStr().equals("extern"));

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

         if (  tmp->Is(CT_COMMA)
            && (tmp->GetLevel() == (paren_open->GetLevel() + 1)))
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
         pc->SetType(CT_FUNC_CTOR_VAR);
         LOG_FMT(LFCN, "%s(%d):   3) Marked Text() '%s' as FUNC_CTOR_VAR on orig line %zu, orig col %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      }
      else if (pc->GetBraceLevel() > 0)
      {
         Chunk *br_open = pc->GetPrevType(CT_BRACE_OPEN, pc->GetBraceLevel() - 1);

         if (  br_open->IsNotNullChunk()
            && br_open->GetParentType() != CT_EXTERN
            && br_open->GetParentType() != CT_NAMESPACE)
         {
            // Do a check to see if the level is right
            prev = pc->GetPrevNcNnlNi();   // Issue #2279

            if (  !prev->IsString("*")
               && !prev->IsString("&"))
            {
               Chunk *p_op = pc->GetPrevType(CT_BRACE_OPEN, pc->GetBraceLevel() - 1);

               if (  p_op->IsNotNullChunk()
                  && p_op->GetParentType() != CT_CLASS
                  && p_op->GetParentType() != CT_STRUCT
                  && p_op->GetParentType() != CT_NAMESPACE)
               {
                  pc->SetType(CT_FUNC_CTOR_VAR);
                  LOG_FMT(LFCN, "%s(%d):   4) Marked Text() is'%s', as FUNC_CTOR_VAR on orig line %zu, orig col %zu\n",
                          __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
               }
            }
         }
      }
   }

   if (semi->IsNotNullChunk())
   {
      semi->SetParentType(pc->GetType());
   }

   // Issue # 1403, 2152
   if (paren_open->GetPrev()->Is(CT_FUNC_CTOR_VAR))
   {
      flag_parens(paren_open, PCF_IN_FCN_CTOR, CT_FPAREN_OPEN, pc->GetType(), false);
   }
   else
   {
      // see also Issue #2103
      Chunk *funtionName = paren_open->GetPrevNcNnl();                    // Issue #3967
      Chunk *a           = funtionName->GetPrevNcNnl();

      while (a->IsNotNullChunk())
      {
         LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s, parent type is %s\n",
                 __func__, __LINE__, a->GetOrigLine(), a->GetOrigCol(), a->Text(),
                 get_token_name(a->GetType()), get_token_name(a->GetParentType()));
         log_pcf_flags(LFCN, a->GetFlags());

         if (  a->Is(CT_ARITH)
            && (strcmp(a->Text(), "&") == 0))
         {
            a->SetType(CT_BYREF);
         }

         if (a->GetParentType() == CT_NONE)
         {
            a->SetParentType(CT_FUNC_DEF);
         }
         // if token has PCF_STMT_START set, exit the loop
         PcfFlags f = a->GetFlags();
         PcfFlags u = f & PCF_STMT_START;
         bool     b = u != E_PcfFlag::PCF_NONE;

         if (b)
         {
            break;
         }
         a = a->GetPrevNcNnl();
      }
      flag_parens(paren_open, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, pc->GetType(), false);
   }

   if (pc->Is(CT_FUNC_CTOR_VAR))
   {
      pc->SetFlagBits(PCF_VAR_1ST_DEF);
      return;
   }

   if (next->Is(CT_TSQUARE))
   {
      next = next->GetNextNcNnl();

      if (next->IsNullChunk())
      {
         return;
      }
   }
   // Mark parameters and return type
   fix_fcn_def_params(next);
   mark_function_return_type(pc, pc->GetPrevNcNnlNi(), pc->GetType());   // Issue #2279

   /* mark C# where chunk */
   if (  language_is_set(lang_flag_e::LANG_CS)
      && (  (pc->Is(CT_FUNC_DEF))
         || (pc->Is(CT_FUNC_PROTO))))
   {
      tmp = paren_close->GetNextNcNnl();
      PcfFlags in_where_spec_flags = PCF_NONE;

      while (  tmp->IsNotNullChunk()
            && tmp->IsNot(CT_BRACE_OPEN)
            && tmp->IsNot(CT_SEMICOLON))
      {
         mark_where_chunk(tmp, pc->GetType(), tmp->GetFlags() | in_where_spec_flags);
         in_where_spec_flags = tmp->GetFlags() & PCF_IN_WHERE_SPEC;

         tmp = tmp->GetNextNcNnl();
      }
   }

   // Find the brace pair and set the parent
   if (pc->Is(CT_FUNC_DEF))
   {
      tmp = paren_close->GetNextNcNnl();

      while (  tmp->IsNotNullChunk()
            && tmp->IsNot(CT_BRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): (13) SET TO CT_FUNC_DEF: orig line is %zu, orig col is %zu, Text() '%s'\n",
                 __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());
         tmp->SetParentType(CT_FUNC_DEF);

         if (!tmp->IsSemicolon())
         {
            tmp->SetFlagBits(PCF_OLD_FCN_PARAMS);
         }
         tmp = tmp->GetNextNcNnl();
      }

      if (tmp->Is(CT_BRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): (14) SET TO CT_FUNC_DEF: orig line is %zu, orig col is %zu, Text() '%s'\n",
                 __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());
         tmp->SetParentType(CT_FUNC_DEF);
         tmp = tmp->GetClosingParen();

         if (tmp->IsNotNullChunk())
         {
            LOG_FMT(LFCN, "%s(%d): (15) SET TO CT_FUNC_DEF: orig line is %zu, orig col is %zu, Text() '%s'\n",
                    __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(), tmp->Text());
            tmp->SetParentType(CT_FUNC_DEF);
         }
      }
   }
} // mark_function


bool mark_function_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTYPE, "%s(%d): type %s, text '%s' @ orig line %zu, orig col %zu\n",
           __func__, __LINE__, get_token_name(pc->GetType()), pc->Text(),
           pc->GetOrigLine(), pc->GetOrigCol());

   size_t  star_count = 0;
   size_t  word_count = 0;
   Chunk   *ptrcnk    = Chunk::NullChunkPtr;
   Chunk   *tmp;
   Chunk   *apo;
   Chunk   *apc;
   Chunk   *aft;
   bool    anon = false;
   E_Token pt;
   E_Token ptp;

   // Scan backwards across the name, which can only be a word and single star
   Chunk *varcnk = pc->GetPrevNcNnlNi();   // Issue #2279
   LOG_FMT(LFTYPE, "%s(%d): varcnk: text '%s', type %s, @ orig line %zu:, orig col %zu\n",
           __func__, __LINE__, varcnk->Text(), get_token_name(varcnk->GetType()),
           varcnk->GetOrigLine(), varcnk->GetOrigCol());

   varcnk = varcnk->GetPrevNbsb();
   LOG_FMT(LFTYPE, "%s(%d): varcnk: text '%s', type %s, @ orig line %zu:, orig col %zu\n",
           __func__, __LINE__, varcnk->Text(), get_token_name(varcnk->GetType()),
           varcnk->GetOrigLine(), varcnk->GetOrigCol());

   if (  varcnk->IsNotNullChunk()
      && !varcnk->IsWord())
   {
      if (  language_is_set(lang_flag_e::LANG_OC)
         && varcnk->IsString("^")
         && varcnk->GetPrevNcNnlNi()->IsParenOpen())   // Issue #2279
      {
         // anonymous ObjC block type -- RTYPE (^)(ARGS)
         anon = true;
      }
      else
      {
         LOG_FMT(LFTYPE, "%s(%d): not a word: text '%s', type %s, @ orig line %zu:, orig col %zu\n",
                 __func__, __LINE__, varcnk->Text(), get_token_name(varcnk->GetType()),
                 varcnk->GetOrigLine(), varcnk->GetOrigCol());
         goto nogo_exit;
      }
   }
   LOG_FMT(LFTYPE, "%s(%d): pc: text is '%s', type is %s, @ orig line is %zu:, orig col is %zu\n",
           __func__, __LINE__, pc->Text(), get_token_name(pc->GetType()),
           pc->GetOrigLine(), pc->GetOrigCol());
   apo = pc->GetNextNcNnl();
   LOG_FMT(LFTYPE, "%s(%d): apo: text is '%s', type is %s, @ orig line is %zu:, orig col is %zu\n",
           __func__, __LINE__, apo->Text(), get_token_name(apo->GetType()),
           apo->GetOrigLine(), apo->GetOrigCol());

   if (apo->IsNullChunk())
   {
      return(false);
   }
   apc = apo->GetClosingParen();
   LOG_FMT(LFTYPE, "%s(%d): apc: text is '%s', type is %s, @ orig line is %zu:, orig col is %zu\n",
           __func__, __LINE__, apc->Text(), get_token_name(apc->GetType()),
           apc->GetOrigLine(), apc->GetOrigCol());

   if (  apc->IsNotNullChunk()
      && (  !apo->IsParenOpen()
         || ((apc = apo->GetClosingParen())->IsNullChunk())))
   {
      LOG_FMT(LFTYPE, "%s(%d): not followed by parens\n", __func__, __LINE__);
      goto nogo_exit;
   }
   LOG_FMT(LFTYPE, "%s(%d): apc: text is '%s', type is %s, @ orig line is %zu:, orig col is %zu\n",
           __func__, __LINE__, apc->Text(), get_token_name(apc->GetType()),
           apc->GetOrigLine(), apc->GetOrigCol());
   aft = apc->GetNextNcNnl();
   LOG_FMT(LFTYPE, "%s(%d): aft: text is '%s', type is %s, @ orig line is %zu:, orig col is %zu\n",
           __func__, __LINE__, aft->Text(), get_token_name(aft->GetType()),
           aft->GetOrigLine(), aft->GetOrigCol());

   if (aft->Is(CT_BRACE_OPEN))
   {
      pt = CT_FUNC_DEF;
   }
   else if (  aft->Is(CT_SEMICOLON)
           || aft->Is(CT_ASSIGN)
           || aft->Is(CT_COMMA)                       // Issue #3259
           || aft->Is(CT_FPAREN_CLOSE))               // Issue #3259
   {
      pt = CT_FUNC_PROTO;
   }
   else
   {
      LOG_FMT(LFTYPE, "%s(%d): not followed by '{' or ';'\n", __func__, __LINE__);
      goto nogo_exit;
   }
   ptp = pc->TestFlags(PCF_IN_TYPEDEF) ? CT_FUNC_TYPE : CT_FUNC_VAR;

   tmp = pc;

   while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
   {
      tmp = tmp->GetPrevNbsb();
      LOG_FMT(LFTYPE, "%s(%d):  -- type is %s, %s on orig line %zu, orig col is %zu",             // Issue #3259
              __func__, __LINE__,
              get_token_name(tmp->GetType()), tmp->Text(),
              tmp->GetOrigLine(), tmp->GetOrigCol());

      if (tmp->IsSemicolon())
      {
         // Stop if we found previous statement. Make 'tmp' null to make sure
         // chunk_ends_type() does not start from the previous statement
         LOG_FMT(LFTYPE, " -- found semicolon (break)\n");
         tmp = Chunk::NullChunkPtr;
         break;
      }
      else if (  tmp->IsStar()
              || tmp->Is(CT_PTR_TYPE)
              || tmp->Is(CT_CARET))
      {
         star_count++;
         ptrcnk = tmp;
         LOG_FMT(LFTYPE, " -- PTR_TYPE\n");
      }
      else if (  tmp->IsWord()
              || tmp->Is(CT_WORD)
              || tmp->Is(CT_TYPE))
      {
         word_count++;
         LOG_FMT(LFTYPE, " -- TYPE(%s)\n", tmp->Text());
      }
      else if (tmp->Is(CT_DC_MEMBER))
      {
         word_count = 0;
         LOG_FMT(LFTYPE, " -- :: reset word_count\n");
      }
      else if (tmp->IsString("("))
      {
         LOG_FMT(LFTYPE, " -- open paren (break)\n");
         break;
      }
      else
      {
         LOG_FMT(LFTYPE, "%s(%d): --  unexpected token: type is %s, Text() '%s', on orig line %zu, orig col %zu\n",
                 __func__, __LINE__,
                 get_token_name(tmp->GetType()), tmp->Text(),
                 tmp->GetOrigLine(), tmp->GetOrigCol());
         goto nogo_exit;
      }
   }

   // Fixes #issue 1577
   // Allow word count 2 in case of function pointer declaration.
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

   if (ptrcnk->IsNotNullChunk())
   {
      ptrcnk->SetType(CT_PTR_TYPE);
   }

   if (!anon)
   {
      if (pc->TestFlags(PCF_IN_TYPEDEF))
      {
         varcnk->SetType(CT_FUNC_TYPE);   // Issue #3402
      }
      else
      {
         varcnk->SetType(CT_FUNC_VAR);
         varcnk->SetFlagBits(PCF_VAR_1ST_DEF);
      }
   }
   pc->SetType(CT_TPAREN_CLOSE);
   pc->SetParentType(ptp);

   apo->SetType(CT_FPAREN_OPEN);
   apo->SetParentType(pt);
   apc->SetType(CT_FPAREN_CLOSE);
   apc->SetParentType(pt);
   fix_fcn_def_params(apo);
   flag_parens(apo, PCF_IN_FCN_DEF, CT_NONE, pt, false);

   if (aft->IsSemicolon())
   {
      aft->SetParentType(aft->TestFlags(PCF_IN_TYPEDEF) ? CT_TYPEDEF : CT_FUNC_VAR);
   }
   else if (aft->Is(CT_BRACE_OPEN))
   {
      flag_parens(aft, PCF_NONE, CT_NONE, pt, false);
   }
   // Step backwards to the previous open paren and mark everything a
   tmp = pc;

   while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
   {
      LOG_FMT(LFTYPE, "%s(%d):  ++ type is %s, Text() '%s', on orig line %zu, orig col %zu\n",
              __func__, __LINE__, get_token_name(tmp->GetType()), tmp->Text(),
              tmp->GetOrigLine(), tmp->GetOrigCol());

      LOG_FMT(LFTYPE, "%s(%d):  ++ unbekannt: '%d'\n",
              __func__, __LINE__, *tmp->GetStr().c_str());
      log_pcf_flags(LFTYPE, pc->GetFlags());

      if (tmp->IsParenOpen())                          // 3259 ??
      {
         //if (!pc->TestFlags(PCF_IN_TYPEDEF))
         if (  !tmp->TestFlags(PCF_IN_TYPEDEF)                   // Issue #3259
            && !tmp->TestFlags(PCF_IN_FCN_DEF))                  // Issue #3259
         {
            tmp->SetFlagBits(PCF_VAR_1ST_DEF);
         }
         tmp->SetType(CT_TPAREN_OPEN);
         tmp->SetParentType(ptp);

         tmp = tmp->GetPrevNcNnlNi();   // Issue #2279

         if (  tmp->Is(CT_FUNCTION)
            || tmp->Is(CT_FUNC_CALL)
            || tmp->Is(CT_FUNC_CALL_USER)
            || tmp->Is(CT_FUNC_DEF)
            || tmp->Is(CT_FUNC_PROTO))
         {
            tmp->SetType(CT_TYPE);
            tmp->ResetFlagBits(PCF_VAR_1ST_DEF);
         }
         mark_function_return_type(varcnk, tmp, ptp);
         break;
      }
   }
   return(true);

nogo_exit:
   tmp = pc->GetNextNcNnl();

   if (tmp->IsParenOpen())
   {
      LOG_FMT(LFTYPE, "%s(%d): setting FUNC_CALL on orig line is %zu, orig col is %zu\n",
              __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol());
      flag_parens(tmp, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
   }
   return(false);
} // mark_function_type


void mark_lvalue(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->TestFlags(PCF_IN_PREPROC))
   {
      return;
   }

   for (Chunk *prev = pc->GetPrevNcNnlNi();  // Issue #2279
        prev->IsNotNullChunk();
        prev = prev->GetPrevNcNnlNi())       // Issue #2279
   {
      if (  prev->GetLevel() < pc->GetLevel()
         || prev->Is(CT_ACCESS_COLON)
         || prev->Is(CT_ASSIGN)
         || prev->Is(CT_BOOL)
         || prev->Is(CT_COMMA)
         || prev->IsCppInheritanceAccessSpecifier()
         || prev->IsSemicolon()
         || prev->IsString("(")
         || prev->IsString("{")
         || prev->IsString("[")
         || prev->TestFlags(PCF_IN_PREPROC)
         || prev->GetParentType() == CT_NAMESPACE
         || prev->GetParentType() == CT_TEMPLATE)
      {
         break;
      }
      prev->SetFlagBits(PCF_LVALUE);

      if (  prev->GetLevel() == pc->GetLevel()
         && prev->IsString("&"))
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
         && pc->GetLevel() >= start->GetLevel()
         && !(  pc->GetLevel() == start->GetLevel()
             && pc->Is(CT_BRACE_CLOSE)))
   {
      if (  pc->Is(CT_BRACE_OPEN)
         || pc->Is(CT_BRACE_CLOSE)
         || pc->Is(CT_SEMICOLON))
      {
         pc = pc->GetNextNcNnl();

         if (pc->IsNullChunk())
         {
            break;
         }
      }

      if (pc->Is(CT_ALIGN))
      {
         pc = skip_align(pc); // "align(x)" or "align(x):"

         if (pc->IsNullChunk())
         {
            break;
         }
      }
      else if (pc->Is(CT_AMP))
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
   Chunk *angle_close = pc_next->GetNextType(CT_ANGLE_CLOSE, pc->GetLevel());
   Chunk *after       = angle_close->GetNextNcNnl();

   if (after->IsNotNullChunk())
   {
      if (after->IsString("("))
      {
         if (angle_close->TestFlags(PCF_IN_FCN_CALL))
         {
            LOG_FMT(LTEMPFUNC, "%s(%d): marking '%s' in line %zu as a FUNC_CALL\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine());
            LOG_FMT(LFCN, "%s(%d): (16) SET TO CT_FUNC_CALL: orig line is %zu, orig col is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            pc->SetType(CT_FUNC_CALL);
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
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine());
            // its a function!!!
            LOG_FMT(LFCN, "%s(%d): (17) SET TO CT_FUNC_CALL: orig line is %zu, orig col is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            pc->SetType(CT_FUNC_CALL);
            mark_function(pc);
         }
      }
      else if (after->Is(CT_WORD))
      {
         // its a type!
         pc->SetType(CT_TYPE);
         pc->SetFlagBits(PCF_VAR_TYPE);
         after->SetFlagBits(PCF_VAR_DEF);
      }
   }
} // mark_template_func


Chunk *mark_variable_definition(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start->IsNullChunk())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk    *pc   = start;
   PcfFlags flags = PCF_VAR_1ST_DEF;

   LOG_FMT(LVARDEF, "%s(%d): orig line %zu, orig col %zu, Text() '%s', type is %s\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(),
           get_token_name(pc->GetType()));

   // Issue #596
   bool bit_field_colon_is_present = false;

   while (go_on(pc, start))
   {
      if (  pc->Is(CT_WORD)
         || pc->Is(CT_FUNC_CTOR_VAR))
      {
         auto const orig_flags = pc->GetFlags();

         if (!pc->TestFlags(PCF_IN_ENUM))
         {
            pc->SetFlagBits(flags);
         }
         flags &= ~PCF_VAR_1ST;
         LOG_FMT(LVARDEF, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', set PCF_VAR_1ST\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());

         LOG_FMT(LVARDEF,
                 "%s(%d): orig line is %zu, marked Text() '%s'[%s]\n"
                 "   in orig col %zu, flags: %s -> %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->Text(),
                 get_token_name(pc->GetType()), pc->GetOrigCol(),
                 pcf_flags_str(orig_flags).c_str(),
                 pcf_flags_str(pc->GetFlags()).c_str());
      }
      else if (  !bit_field_colon_is_present                      // Issue #2689
              && (  pc->IsStar()
                 || pc->IsMsRef()))
      {
         pc->SetType(CT_PTR_TYPE);
      }
      else if (pc->IsAddress())
      {
         pc->SetType(CT_BYREF);
      }
      else if (  pc->Is(CT_SQUARE_OPEN)
              || pc->Is(CT_ASSIGN))
      {
         pc = skip_expression(pc);
         continue;
      }
      else if (pc->Is(CT_COLON))
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

   if (  var_name->IsNotNullChunk()
      && var_name->GetPrev()->IsNotNullChunk()
      && var_name->GetPrev()->GetType() == CT_DC_MEMBER)
   {
      cs.Push_Back(var_name);
   }

   if (var_name->IsNotNullChunk())
   {
      LOG_FMT(LFCNP, "%s(%d): parameter on orig line %zu, orig col %zu:\n",
              __func__, __LINE__, var_name->GetOrigLine(), var_name->GetOrigCol());

      size_t word_cnt = 0;
      Chunk  *word_type;

      while ((word_type = cs.Pop_Back())->IsNotNullChunk())
      {
         if (  word_type->Is(CT_WORD)
            || word_type->Is(CT_TYPE))
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig line %zu, orig col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->GetOrigLine(), var_name->GetOrigCol(), word_type->Text());
            word_type->SetType(CT_TYPE);
            word_type->SetFlagBits(PCF_VAR_TYPE);
         }
         word_cnt++;
      }

      if (var_name->Is(CT_WORD))
      {
         if (word_cnt > 0)
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig line %zu, orig col %zu: <%s> as VAR\n",
                    __func__, __LINE__, var_name->GetOrigLine(), var_name->GetOrigCol(), var_name->Text());
            var_name->SetFlagBits(PCF_VAR_DEF);
         }
         else
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig line %zu, orig col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->GetOrigLine(), var_name->GetOrigCol(), var_name->Text());
            var_name->SetType(CT_TYPE);
            var_name->SetFlagBits(PCF_VAR_TYPE);
         }
      }
   }
} // mark_variable_stack


PcfFlags mark_where_chunk(Chunk *pc, E_Token parent_type, PcfFlags flags)
{
   /* TODO: should have options to control spacing around the ':' as well as newline ability for the
    * constraint clauses (should it break up a 'where A : B where C : D' on the same line? wrap? etc.) */

   if (pc->Is(CT_WHERE))
   {
      pc->SetType(CT_WHERE_SPEC);
      pc->SetParentType(parent_type);
      flags |= PCF_IN_WHERE_SPEC;
      LOG_FMT(LFTOR, "%s: where-spec on line %zu\n",
              __func__, pc->GetOrigLine());
   }
   else if (flags.test(PCF_IN_WHERE_SPEC))
   {
      if (pc->IsString(":"))
      {
         pc->SetType(CT_WHERE_COLON);
         LOG_FMT(LFTOR, "%s: where-spec colon on line %zu\n",
                 __func__, pc->GetOrigLine());
      }
      else if (pc->IsClassOrStruct())
      {
         /* class/struct inside of a where-clause confuses parser for indentation; set it as a word so it looks like the rest */
         pc->SetType(CT_WORD);
      }
   }

   if (flags.test(PCF_IN_WHERE_SPEC))
   {
      pc->SetFlagBits(PCF_IN_WHERE_SPEC);
   }
   return(flags);
} // mark_where_chunk
