/**
 * @file tokenize_cleanup.cpp
 * Looks at simple sequences to refine the chunk types.
 * Examples:
 *  - change '[' + ']' into '[]'/
 *  - detect "version = 10;" vs "version (xxx) {"
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "char_table.h"
#include "unc_ctype.h"
#include <cstring>

static void check_template(chunk_t *start);


/**
 * Convert '>' + '>' into '>>'
 * If we only have a single '>', then change it to CT_COMPARE.
 */
static chunk_t *handle_double_angle_close(chunk_t *pc)
{
   chunk_t *next = chunk_get_next(pc);

   if (next != NULL)
   {
      if ((pc->type == CT_ANGLE_CLOSE) &&
          (next->type == CT_ANGLE_CLOSE) &&
          (pc->parent_type == CT_NONE) &&
          (memcmp(pc->str, ">>", 2) == 0) &&
          (next->parent_type == CT_NONE))
      {
         pc->str.append('>');
         pc->type = CT_ARITH;
         pc->orig_col_end = next->orig_col_end;

         chunk_t *tmp = chunk_get_next_ncnl(next);
         chunk_del(next);
         next = tmp;
      }
      else
      {
         pc->type = CT_COMPARE;
      }
   }
   return(next);
}


static void split_off_angle_close(chunk_t *pc)
{
   chunk_t nc;

   nc = *pc;

   const chunk_tag_t *ct;

   ct = find_punctuator(pc->text() + 1, cpd.lang_flags);
   if (ct == NULL)
   {
      return;
   }

   pc->str.resize(1);
   pc->orig_col_end = pc->orig_col + 1;
   pc->type = CT_ANGLE_CLOSE;

   nc.type = ct->type;
   nc.str.pop_front();
   nc.orig_col++;
   nc.column++;
   chunk_add_after(&nc, pc);
}


void tokenize_cleanup(void)
{
   chunk_t *pc   = chunk_get_head();
   chunk_t *prev = NULL;
   chunk_t *next;
   chunk_t *tmp;
   chunk_t *tmp2;
   bool    in_type_cast = false;

   /* Since [] is expected to be TSQUARE for the 'operator', we need to make
    * this change in the first pass.
    */
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_SQUARE_OPEN)
      {
         next = chunk_get_next_ncnl(pc);
         if (chunk_is_token(next, CT_SQUARE_CLOSE))
         {
            /* Change '[' + ']' into '[]' */
            pc->type = CT_TSQUARE;
            pc->str  = "[]";
            chunk_del(next);
            pc->orig_col_end += 1;
         }
      }
      if ((pc->type == CT_SEMICOLON) &&
          (pc->flags & PCF_IN_PREPROC) &&
          !chunk_get_next_ncnl(pc, CNAV_PREPROC))
      {
         LOG_FMT(LNOTE, "%s:%d Detected a macro that ends with a semicolon. Possible failures if used.\n",
                 cpd.filename, pc->orig_line);
      }
   }

   /* We can handle everything else in the second pass */
   pc   = chunk_get_head();
   next = chunk_get_next_ncnl(pc);
   while ((pc != NULL) && (next != NULL))
   {
      if ((pc->type == CT_DOT) && ((cpd.lang_flags & LANG_ALLC) != 0))
      {
         pc->type = CT_MEMBER;
      }

      /* Determine the version stuff (D only) */
      if (pc->type == CT_D_VERSION)
      {
         if (next->type == CT_PAREN_OPEN)
         {
            pc->type = CT_D_VERSION_IF;
         }
         else
         {
            if (next->type != CT_ASSIGN)
            {
               LOG_FMT(LERR, "%s:%d %s: version: Unexpected token %s\n",
                       cpd.filename, pc->orig_line, __func__, get_token_name(next->type));
               cpd.error_count++;
            }
            pc->type = CT_WORD;
         }
      }

      /* Determine the scope stuff (D only) */
      if (pc->type == CT_D_SCOPE)
      {
         if (next->type == CT_PAREN_OPEN)
         {
            pc->type = CT_D_SCOPE_IF;
         }
         else
         {
            pc->type = CT_TYPE;
         }
      }

      /**
       * Change CT_BASE before CT_PAREN_OPEN to CT_WORD.
       * public myclass() : base() {
       * }
       */
      if ((pc->type == CT_BASE) && (next->type == CT_PAREN_OPEN))
      {
         pc->type = CT_WORD;
      }

      /**
       * Change CT_WORD after CT_ENUM, CT_UNION, or CT_STRUCT to CT_TYPE
       * Change CT_WORD before CT_WORD to CT_TYPE
       */
      if (next->type == CT_WORD)
      {
         if ((pc->type == CT_ENUM) ||
             (pc->type == CT_UNION) ||
             (pc->type == CT_STRUCT))
         {
            next->type = CT_TYPE;
         }
         if (pc->type == CT_WORD)
         {
            pc->type = CT_TYPE;
         }
      }

      /* change extern to qualifier if extern isn't followed by a string or
       * an open paren
       */
      if (pc->type == CT_EXTERN)
      {
         if (next->type == CT_STRING)
         {
            /* Probably 'extern "C"' */
         }
         else if (next->type == CT_PAREN_OPEN)
         {
            /* Probably 'extern (C)' */
         }
         else
         {
            /* Something else followed by a open brace */
            tmp = chunk_get_next_ncnl(next);
            if ((tmp != NULL) || (tmp->type != CT_BRACE_OPEN))
            {
               pc->type = CT_QUALIFIER;
            }
         }
      }

      /**
       * Change CT_STAR to CT_PTR_TYPE if preceded by CT_TYPE,
       * CT_QUALIFIER, or CT_PTR_TYPE.
       */
      if ((next->type == CT_STAR) &&
          ((pc->type == CT_TYPE) ||
           (pc->type == CT_QUALIFIER) ||
           (pc->type == CT_PTR_TYPE)))
      {
         next->type = CT_PTR_TYPE;
      }

      if ((pc->type == CT_TYPE_CAST) &&
          (next->type == CT_ANGLE_OPEN))
      {
         next->parent_type = CT_TYPE_CAST;
         in_type_cast      = true;
      }

      /**
       * Change angle open/close to CT_COMPARE, if not a template thingy
       */
      if ((pc->type == CT_ANGLE_OPEN) && (pc->parent_type != CT_TYPE_CAST))
      {
         check_template(pc);
      }
      if ((pc->type == CT_ANGLE_CLOSE) && (pc->parent_type != CT_TEMPLATE))
      {
         if (in_type_cast)
         {
            in_type_cast    = false;
            pc->parent_type = CT_TYPE_CAST;
         }
         else
         {
            next = handle_double_angle_close(pc);
         }
      }

      if ((cpd.lang_flags & LANG_D) != 0)
      {
         /* Check for the D string concat symbol '~' */
         if ((pc->type == CT_INV) &&
             ((prev->type == CT_STRING) ||
              (prev->type == CT_WORD) ||
              (next->type == CT_STRING)))
         {
            pc->type = CT_CONCAT;
         }

         /* Check for the D template symbol '!' (word + '!' + word or '(') */
         if ((pc->type == CT_NOT) &&
             (prev->type == CT_WORD) &&
             ((next->type == CT_PAREN_OPEN) ||
              (next->type == CT_WORD) ||
              (next->type == CT_TYPE)))
         {
            pc->type = CT_D_TEMPLATE;
         }

         /* handle "version(unittest) { }" vs "unittest { }" */
         if ((pc->type == CT_UNITTEST) && (prev->type == CT_PAREN_OPEN))
         {
            pc->type = CT_WORD;
         }
      }

      if ((cpd.lang_flags & LANG_CPP) != 0)
      {
         /* Change Word before '::' into a type */
         if ((pc->type == CT_WORD) && (next->type == CT_DC_MEMBER))
         {
            pc->type = CT_TYPE;
         }
      }

      /* Change get/set to CT_WORD if not followed by a brace open */
      if ((pc->type == CT_GETSET) && (next->type != CT_BRACE_OPEN))
      {
         if ((next->type == CT_SEMICOLON) &&
             ((prev->type == CT_BRACE_CLOSE) ||
              (prev->type == CT_BRACE_OPEN) ||
              (prev->type == CT_SEMICOLON)))
         {
            pc->type          = CT_GETSET_EMPTY;
            next->parent_type = CT_GETSET;
         }
         else
         {
            pc->type = CT_WORD;
         }
      }

      /* Interface is only a keyword in MS land if followed by 'class' or 'struct'
       * likewise, 'class' may be a member name in Java.
       */
      if ((pc->type == CT_CLASS) && !CharTable::IsKw1(next->str[0]))
      {
         pc->type = CT_WORD;
      }

      /* Change item after operator (>=, ==, etc) to a CT_OPERATOR_VAL
       * Usually the next item is part of the operator.
       * In a few cases the next few tokens are part of it:
       *  operator +       - common case
       *  operator >>      - need to combine '>' and '>'
       *  operator ()
       *  operator []      - already converted to TSQUARE
       *  operator new []
       *  operator delete []
       *  operator const char *
       *  operator const B&
       *  operator std::allocator<U>
       *
       * In all cases except the last, this will put the entire operator value
       * in one chunk.
       */
      if (pc->type == CT_OPERATOR)
      {
         tmp2 = chunk_get_next(next);
         /* Handle special case of () operator -- [] already handled */
         if (next->type == CT_PAREN_OPEN)
         {
            tmp = chunk_get_next(next);
            if ((tmp != NULL) && (tmp->type == CT_PAREN_CLOSE))
            {
               next->str  = "()";
               next->type = CT_OPERATOR_VAL;
               chunk_del(tmp);
               next->orig_col_end += 1;
            }
         }
         else if ((next->type == CT_ANGLE_CLOSE) &&
                  tmp2 && (tmp2->type == CT_ANGLE_CLOSE) &&
                  (tmp2->orig_col == next->orig_col_end))
         {
            next->str.append('>');
            next->orig_col_end++;
            next->type = CT_OPERATOR_VAL;
            chunk_del(tmp2);
         }
         else if (next->flags & PCF_PUNCTUATOR)
         {
            next->type = CT_OPERATOR_VAL;
         }
         else
         {
            next->type = CT_TYPE;

            /* Replace next with a collection of all tokens that are part of
             * the type.
             */
            tmp2 = next;
            while ((tmp = chunk_get_next(tmp2)) != NULL)
            {
               if ((tmp->type != CT_WORD) &&
                   (tmp->type != CT_TYPE) &&
                   (tmp->type != CT_QUALIFIER) &&
                   (tmp->type != CT_STAR) &&
                   (tmp->type != CT_AMP) &&
                   (tmp->type != CT_TSQUARE))
               {
                  break;
               }
               /* Change tmp into a type so that space_needed() works right */
               make_type(tmp);
               int num_sp = space_needed(tmp2, tmp);
               while (num_sp-- > 0)
               {
                  next->str.append(" ");
               }
               next->str.append(tmp->str);
               tmp2 = tmp;
            }

            while ((tmp2 = chunk_get_next(next)) != tmp)
            {
               chunk_del(tmp2);
            }

            next->type = CT_OPERATOR_VAL;

            next->orig_col_end = next->orig_col + next->len();
         }
         next->parent_type = CT_OPERATOR;

         LOG_FMT(LOPERATOR, "%s: %d:%d operator '%s'\n",
                 __func__, pc->orig_line, pc->orig_col, next->str.c_str());
      }

      /* Change private, public, protected into either a qualifier or label */
      if (pc->type == CT_PRIVATE)
      {
         /* Handle Qt slots - maybe should just check for a CT_WORD? */
         if (chunk_is_str(next, "slots", 5) || chunk_is_str(next, "Q_SLOTS", 7))
         {
            tmp = chunk_get_next(next);
            if ((tmp != NULL) && (tmp->type == CT_COLON))
            {
               next = tmp;
            }
         }
         if (next->type == CT_COLON)
         {
            next->type = CT_PRIVATE_COLON;
            if ((tmp = chunk_get_next_ncnl(next)) != NULL)
            {
               tmp->flags |= PCF_STMT_START | PCF_EXPR_START;
            }
         }
         else
         {
            pc->type = (chunk_is_str(pc, "signals", 7) || chunk_is_str(pc, "Q_SIGNALS", 9)) ? CT_WORD : CT_QUALIFIER;
         }
      }

      /* Look for <newline> 'EXEC' 'SQL' */
      if (chunk_is_str(pc, "EXEC", 4) && chunk_is_str(next, "SQL", 3))
      {
         tmp = chunk_get_prev(pc);
         if (chunk_is_newline(tmp))
         {
            tmp = chunk_get_next(next);
            if (chunk_is_str_case(tmp, "BEGIN", 5))
            {
               pc->type = CT_SQL_BEGIN;
            }
            else if (chunk_is_str_case(tmp, "END", 3))
            {
               pc->type = CT_SQL_END;
            }
            else
            {
               pc->type = CT_SQL_EXEC;
            }

            /* Change words into CT_SQL_WORD until CT_SEMICOLON */
            while (tmp != NULL)
            {
               if (tmp->type == CT_SEMICOLON)
               {
                  break;
               }
               if ((tmp->len() > 0) && isalpha(*tmp->str))
               {
                  tmp->type = CT_SQL_WORD;
               }
               tmp = chunk_get_next_ncnl(tmp);
            }
         }
      }

      /* handle MS abomination 'for each' */
      if ((pc->type == CT_FOR) && chunk_is_str(next, "each", 4) &&
          (next == chunk_get_next(pc)))
      {
         /* merge the two with a space between */
         pc->str.append(' ');
         pc->str += next->str;
         pc->orig_col_end = next->orig_col_end;
         chunk_del(next);
         next = chunk_get_next_ncnl(pc);
         /* label the 'in' */
         if (next && (next->type == CT_PAREN_OPEN))
         {
            tmp = chunk_get_next_ncnl(next);
            while (tmp && (tmp->type != CT_PAREN_CLOSE))
            {
               if (chunk_is_str(tmp, "in", 2))
               {
                  tmp->type = CT_IN;
                  break;
               }
               tmp = chunk_get_next_ncnl(tmp);
            }
         }
      }

      /* ObjectiveC allows keywords to be used as identifiers in some situations
       * This is a dirty hack to allow some of the more common situations.
       */
      if (cpd.lang_flags & LANG_OC)
      {
         if (((pc->type == CT_IF) ||
             (pc->type == CT_FOR) ||
             (pc->type == CT_WHILE)) &&
             !chunk_is_token(next, CT_PAREN_OPEN))
         {
            pc->type = CT_WORD;
         }
         if ((pc->type == CT_DO) &&
             (chunk_is_token(prev, CT_MINUS) ||
              chunk_is_token(next, CT_SQUARE_CLOSE)))
         {
            pc->type = CT_WORD;
         }
      }

      /* Another hack to clean up more keyword abuse */
      if ((pc->type == CT_CLASS) &&
          (chunk_is_token(prev, CT_DOT) ||
           chunk_is_token(next, CT_DOT)))
      {
         pc->type = CT_WORD;
      }

      /* Detect Objective C class name */
      if ((pc->type == CT_OC_IMPL) ||
          (pc->type == CT_OC_INTF) ||
          (pc->type == CT_OC_PROTOCOL))
      {
         if (next->type != CT_PAREN_OPEN)
         {
            next->type = CT_OC_CLASS;
         }
         next->parent_type = pc->type;

         tmp = chunk_get_next_ncnl(next);
         if (tmp != NULL)
         {
            tmp->flags |= PCF_STMT_START | PCF_EXPR_START;
         }

         tmp = chunk_get_next_type(pc, CT_OC_END, pc->level);
         if (tmp != NULL)
         {
            tmp->parent_type = pc->type;
         }
      }

      if (pc->type == CT_OC_INTF)
      {
         tmp = chunk_get_next_ncnl(pc, CNAV_PREPROC);
         while ((tmp != NULL) && (tmp->type != CT_OC_END))
         {
            if (get_token_pattern_class(tmp->type) != PATCLS_NONE)
            {
               LOG_FMT(LOBJCWORD, "@interface %d:%d change '%s' (%s) to CT_WORD\n",
                       pc->orig_line, pc->orig_col, tmp->str.c_str(),
                       get_token_name(tmp->type));
               tmp->type = CT_WORD;
            }
            tmp = chunk_get_next_ncnl(tmp, CNAV_PREPROC);
         }
      }

      /* Detect Objective-C categories and class extensions */
      /* @interface ClassName (CategoryName) */
      /* @implementation ClassName (CategoryName) */
      /* @interface ClassName () */
      /* @implementation ClassName () */
      if (((pc->parent_type == CT_OC_IMPL) ||
           (pc->parent_type == CT_OC_INTF) ||
           (pc->type == CT_OC_CLASS)) &&
          (next->type == CT_PAREN_OPEN))
      {
         next->parent_type = pc->parent_type;

         tmp = chunk_get_next(next);
         if ((tmp != NULL) && (tmp->next != NULL))
         {
            if (tmp->type == CT_PAREN_CLOSE)
            {
               //tmp->type        = CT_OC_CLASS_EXT;
               tmp->parent_type = pc->parent_type;
            }
            else
            {
               tmp->type        = CT_OC_CATEGORY;
               tmp->parent_type = pc->parent_type;
            }
         }

         tmp = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
         if (tmp != NULL)
         {
            tmp->parent_type = pc->parent_type;
         }
      }

      /**
       * Objective C @dynamic and @synthesize
       *  @dynamic xxx, yyy;
       *  @synthesize xxx, yyy;
       * Just find the semicolon and mark it.
       */
      if (pc->type == CT_OC_DYNAMIC)
      {
         tmp = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
         if (tmp != NULL)
         {
            tmp->parent_type = pc->type;
         }
      }

      /* Detect Objective C @property
       *  @property NSString *stringProperty;
       *  @property(nonatomic, retain) NSMutableDictionary *shareWith;
       */
      if (pc->type == CT_OC_PROPERTY)
      {
         if (next->type != CT_PAREN_OPEN)
         {
            next->flags |= PCF_STMT_START | PCF_EXPR_START;
         }
         else
         {
            next->parent_type = pc->type;

            tmp = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
            if (tmp != NULL)
            {
               tmp->parent_type = pc->type;
               tmp = chunk_get_next_ncnl(tmp);
               if (tmp != NULL)
               {
                  tmp->flags |= PCF_STMT_START | PCF_EXPR_START;

                  tmp = chunk_get_next_type(tmp, CT_SEMICOLON, pc->level);
                  if (tmp != NULL)
                  {
                     tmp->parent_type = pc->type;
                  }
               }
            }
         }
      }

      /* Detect Objective C @selector
       *  @selector(msgNameWithNoArg)
       *  @selector(msgNameWith1Arg:)
       *  @selector(msgNameWith2Args:arg2Name:)
       */
      if ((pc->type == CT_OC_SEL) && (next->type == CT_PAREN_OPEN))
      {
         next->parent_type = pc->type;

         tmp = chunk_get_next(next);
         if (tmp != NULL)
         {
            tmp->type        = CT_OC_SEL_NAME;
            tmp->parent_type = pc->type;

            while ((tmp = chunk_get_next_ncnl(tmp)) != NULL)
            {
               if (tmp->type == CT_PAREN_CLOSE)
               {
                  tmp->parent_type = CT_OC_SEL;
                  break;
               }
               tmp->type        = CT_OC_SEL_NAME;
               tmp->parent_type = pc->type;
            }
         }
      }

      /* Mark Objective-C blocks (aka lambdas or closures)
       *  The syntax and usage is exactly like C function pointers with two exceptions:
       *  Instead of an asterisk they have a caret as pointer symbol.
       *  In method declarations which take a block as parameter, there can be anonymous blocks, e.g.: (^)
       *  1. block literal: ^{ ... };
       *  2. block declaration: return_t (^name) (int arg1, int arg2, ...) NB: return_t is optional and name can be optional if found as param in a method declaration.
       *  3. block expression: ^ return_t (int arg) { ... }; NB: return_t is optional
       *
       *  See http://developer.apple.com/mac/library/documentation/Cocoa/Conceptual/Blocks for more info...
       */
      if ((cpd.lang_flags & LANG_OC) &&
          (pc->type == CT_ARITH) &&
          (chunk_is_str(pc, "^", 1)) &&
          (prev->type != CT_NUMBER) &&
          (prev->type != CT_NUMBER_FP))
      {
         /* mark objc blocks caret so that we can process it later*/
         pc->type = CT_OC_BLOCK_CARET;

         if (prev->type == CT_PAREN_OPEN)
         {
            /* block declaration */
            pc->parent_type = CT_OC_BLOCK_TYPE;
         }
         else if ((next->type == CT_PAREN_OPEN) ||
                  (next->type == CT_BRACE_OPEN))
         {
            /* block expression without return type */
            /* block literal */
            pc->parent_type = CT_OC_BLOCK_EXPR;
         }
         else
         {
            /* block expression with return type (seldomly used) */
            if (prev->type == CT_ASSIGN)
            {
               /* shortcut to spare the peeking below
                * the XOR operator wouldn't be used directly
                * after an assign all by itself */
               pc->parent_type = CT_OC_BLOCK_EXPR;
            }
            else
            {
               /* this ones tricky because we don't know how many
                * stars the return type has - if there even is one */
               tmp = pc;
               while ((tmp = chunk_get_next(tmp)) != NULL)
               {
                  /* we just peek ahead and see if the line contains
                   * an open brace somewhere.
                   * FIXME: this check needs to be more thorough. */
                  if (tmp->type == CT_BRACE_OPEN)
                  {
                     pc->parent_type = CT_OC_BLOCK_EXPR;
                     break;
                  }
               }
            }
         }
      }

      /* Handle special preprocessor junk */
      if (pc->type == CT_PREPROC)
      {
         pc->parent_type = next->type;
      }

      /* Detect "pragma region" and "pragma endregion" */
      if ((pc->type == CT_PP_PRAGMA) && (next->type == CT_PREPROC_BODY))
      {
         if ((memcmp(next->str, "region", 6) == 0) ||
             (memcmp(next->str, "endregion", 9) == 0))
         {
            pc->type = (*next->str == 'r') ? CT_PP_REGION : CT_PP_ENDREGION;

            prev->parent_type = pc->type;
         }
      }

      /* Check for C# nullable types '?' is in next */
      if ((cpd.lang_flags & LANG_CS) &&
          (next->type == CT_QUESTION) &&
          (next->orig_col == (pc->orig_col + pc->len())))
      {
         tmp = chunk_get_next_ncnl(next);
         if (tmp != NULL)
         {
            bool doit = ((tmp->type == CT_PAREN_CLOSE) ||
                         (tmp->type == CT_ANGLE_CLOSE));

            if (tmp->type == CT_WORD)
            {
               tmp2 = chunk_get_next_ncnl(tmp);
               if ((tmp2 != NULL) &&
                   ((tmp2->type == CT_SEMICOLON) ||
                    (tmp2->type == CT_ASSIGN) ||
                    (tmp2->type == CT_COMMA)))
               {
                  doit = true;
               }
            }

            if (doit)
            {
               pc->str += next->str;
               chunk_del(next);
               next = tmp;
            }
         }
      }

      /* Change 'default(' into a sizeof-like statement */
      if ((cpd.lang_flags & LANG_CS) &&
          (pc->type == CT_DEFAULT) &&
          (next->type == CT_PAREN_OPEN))
      {
         pc->type = CT_SIZEOF;
      }

      if ((pc->type == CT_UNSAFE) && (next->type != CT_BRACE_OPEN))
      {
         pc->type = CT_QUALIFIER;
      }

      if ((pc->type == CT_USING) && (next->type == CT_PAREN_OPEN))
      {
         pc->type = CT_USING_STMT;
      }

      /* Add minimal support for C++0x rvalue references */
      if ((pc->type == CT_BOOL) && chunk_is_str(pc, "&&", 2))
      {
         if (prev->type == CT_TYPE)
         {
            pc->type = CT_BYREF;
         }
      }

      /* HACK: treat try followed by a colon as a qualifier to handle this:
       *   A::A(int) try : B() { } catch (...) { }
       */
      if ((pc->type == CT_TRY) && chunk_is_str(pc, "try", 3) &&
          (next != NULL) && (next->type == CT_COLON))
      {
         pc->type = CT_QUALIFIER;
      }

      /* TODO: determine other stuff here */

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(pc);
   }
}


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 */
static void check_template(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *end;
   chunk_t *prev;
   chunk_t *next;
   bool    in_if = false;

   LOG_FMT(LTEMPL, "%s: Line %d, col %d:", __func__, start->orig_line, start->orig_col);

   prev = chunk_get_prev_ncnl(start, CNAV_PREPROC);
   if (prev == NULL)
   {
      return;
   }

   if (prev->type == CT_TEMPLATE)
   {
      LOG_FMT(LTEMPL, " CT_TEMPLATE:");

      /* We have: "template< ... >", which is a template declaration */
      int level = 1;
      for (pc = chunk_get_next_ncnl(start, CNAV_PREPROC);
           pc != NULL;
           pc = chunk_get_next_ncnl(pc, CNAV_PREPROC))
      {
         LOG_FMT(LTEMPL, " [%s,%d]", get_token_name(pc->type), level);

         if ((pc->str[0] == '>') && (pc->len() > 1))
         {
            LOG_FMT(LTEMPL, " {split '%s' at %d:%d}",
                    pc->str.c_str(), pc->orig_line, pc->orig_col);
            split_off_angle_close(pc);
         }

         if (chunk_is_str(pc, "<", 1))
         {
            level++;
         }
         else if (chunk_is_str(pc, ">", 1))
         {
            level--;
            if (level == 0)
            {
               break;
            }
         }
      }
      end = pc;
   }
   else
   {
      /* We may have something like "a< ... >", which is a template where
       * '...' may consist of anything except braces {}, a semicolon, and
       * unbalanced parens.
       * if we are inside an 'if' statement and hit a CT_BOOL, then it isn't a
       * template.
       */

      /* A template requires a word/type right before the open angle */
      if ((prev->type != CT_WORD) &&
          (prev->type != CT_TYPE) &&
          (prev->type != CT_OPERATOR_VAL) &&
          (prev->parent_type != CT_OPERATOR))
      {
         LOG_FMT(LTEMPL, " - after %s + ( - Not a template\n", get_token_name(prev->type));
         start->type = CT_COMPARE;
         return;
      }

      LOG_FMT(LTEMPL, " - prev %s -", get_token_name(prev->type));

      /* Scan back and make sure we aren't inside square parens */
      pc = start;
      while ((pc = chunk_get_prev_ncnl(pc, CNAV_PREPROC)) != NULL)
      {
         if ((pc->type == CT_SEMICOLON) ||
             (pc->type == CT_BRACE_OPEN) ||
             (pc->type == CT_BRACE_CLOSE) ||
             (pc->type == CT_SQUARE_CLOSE))
         {
            break;
         }
         if ((pc->type == CT_IF) || (pc->type == CT_RETURN))
         {
            in_if = true;
            break;
         }
         if (pc->type == CT_SQUARE_OPEN)
         {
            LOG_FMT(LTEMPL, " - Not a template: after a square open\n");
            start->type = CT_COMPARE;
            return;
         }
      }

      /* Scan forward to the angle close
       * If we have a comparison in there, then it can't be a template.
       */
      c_token_t tokens[1024];
      int       num_tokens = 1;

      tokens[0] = CT_ANGLE_OPEN;
      for (pc = chunk_get_next_ncnl(start, CNAV_PREPROC);
           pc != NULL;
           pc = chunk_get_next_ncnl(pc, CNAV_PREPROC))
      {
         LOG_FMT(LTEMPL, " [%s,%d]", get_token_name(pc->type), num_tokens);

         if ((tokens[num_tokens - 1] == CT_ANGLE_OPEN) &&
             (pc->str[0] == '>') && (pc->len() > 1) &&
             (cpd.settings[UO_tok_split_gte].b || chunk_is_str(pc, ">>", 2)))
         {
            LOG_FMT(LTEMPL, " {split '%s' at %d:%d}",
                    pc->str.c_str(), pc->orig_line, pc->orig_col);
            split_off_angle_close(pc);
         }

         if (chunk_is_str(pc, "<", 1))
         {
            tokens[num_tokens++] = CT_ANGLE_OPEN;
         }
         else if (chunk_is_str(pc, ">", 1))
         {
            if ((num_tokens > 0) && (tokens[num_tokens - 1] == CT_PAREN_OPEN))
            {
               handle_double_angle_close(pc);
            }
            else if (--num_tokens <= 0)
            {
               break;
            }
            else if (tokens[num_tokens] != CT_ANGLE_OPEN)
            {
               /* unbalanced parens */
               break;
            }
         }
         else if (in_if &&
                  ((pc->type == CT_BOOL) ||
                   (pc->type == CT_COMPARE)))
         {
            break;
         }
         else if ((pc->type == CT_BRACE_OPEN) ||
                  (pc->type == CT_BRACE_CLOSE) ||
                  (pc->type == CT_SEMICOLON))
         {
            break;
         }
         else if (pc->type == CT_PAREN_OPEN)
         {
            if (num_tokens >= (int)(ARRAY_SIZE(tokens) - 1))
            {
               break;
            }
            tokens[num_tokens++] = pc->type;
         }
         else if (pc->type == CT_PAREN_CLOSE)
         {
            num_tokens--;
            if (tokens[num_tokens] != (pc->type - 1))
            {
               /* unbalanced parens */
               break;
            }
         }
      }
      end = pc;
   }

   if ((end != NULL) && (end->type == CT_ANGLE_CLOSE))
   {
      pc = chunk_get_next_ncnl(end, CNAV_PREPROC);
      if ((pc != NULL) && (pc->type != CT_NUMBER))
      {
         LOG_FMT(LTEMPL, " - Template Detected\n");

         start->parent_type = CT_TEMPLATE;

         pc = start;
         while (pc != end)
         {
            next       = chunk_get_next_ncnl(pc, CNAV_PREPROC);
            pc->flags |= PCF_IN_TEMPLATE;
            if (next->type != CT_PAREN_OPEN)
            {
               make_type(pc);
            }
            pc = next;
         }
         end->parent_type = CT_TEMPLATE;
         end->flags      |= PCF_IN_TEMPLATE;
         return;
      }
   }

   LOG_FMT(LTEMPL, " - Not a template: end = %s\n",
           (end != NULL) ? get_token_name(end->type) : "<null>");
   start->type = CT_COMPARE;
}
