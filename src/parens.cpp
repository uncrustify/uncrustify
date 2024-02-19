/**
 * @file parens.cpp
 * Adds or removes parens.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "parens.h"

#include "log_rules.h"


constexpr static auto LCURRENT = LPARADD;


using namespace uncrustify;


//! Add an open parenthesis after first and add a close parenthesis before the last
static void add_parens_between(Chunk *first, Chunk *last);


/**
 * Scans between two parens and adds additional parens if needed.
 * This function is recursive. If it hits another open paren, it'll call itself
 * with the new bounds.
 *
 * Adds optional parens in an IF or SWITCH conditional statement.
 *
 * This basically just checks for a CT_COMPARE that isn't surrounded by parens.
 * The edges for the compare are the open, close and any CT_BOOL tokens.
 *
 * This only handles VERY simple patterns:
 *   (!a && b)         => (!a && b)          -- no change
 *   (a && b == 1)     => (a && (b == 1))
 *   (a == 1 || b > 2) => ((a == 1) || (b > 2))
 *
 * FIXME: we really should bail if we transition between a preprocessor and
 *        a non-preprocessor
 */
static void check_bool_parens(Chunk *popen, Chunk *pclose, int nest);


void do_parens()
{
   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_if_bool");

   if (options::mod_full_paren_if_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while ((pc = pc->GetNextNcNnl())->IsNotNullChunk())
      {
         if (  pc->IsNot(CT_SPAREN_OPEN)
            || (  pc->GetParentType() != CT_IF
               && pc->GetParentType() != CT_ELSEIF
               && pc->GetParentType() != CT_SWITCH))
         {
            continue;
         }
         // Grab the close sparen
         Chunk *pclose = pc->GetNextType(CT_SPAREN_CLOSE, pc->GetLevel(), E_Scope::PREPROC);

         if (pclose->IsNotNullChunk())
         {
            check_bool_parens(pc, pclose, 0);
            pc = pclose;
         }
      }
   }
} // do_parens


void do_parens_assign()                         // Issue #3316
{
   LOG_FUNC_ENTRY();

   // Whether to fully parenthesize Boolean expressions after '='
   // statement, as in 'x = a && b > c;' => 'x = (a && (b > c));'.
   log_rule_B("mod_full_paren_assign_bool");

   if (options::mod_full_paren_assign_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while ((pc = pc->GetNextNcNnl())->IsNotNullChunk())
      {
         if (pc->Is(CT_ASSIGN))
         {
            if (pc->TestFlags(PCF_IN_SPAREN))            // Issue #4239
            {
               continue;
            }
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, orig col is %zu, text is '%s', level is %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), pc->GetLevel());
            // look before for an open sparen
            size_t check_level = pc->GetLevel();
            Chunk  *p          = pc->GetPrevNc(E_Scope::PREPROC);

            while (p->IsNotNullChunk())
            {
               LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                       __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

               //log_pcf_flags(LPARADD, p->GetFlags());
               if (p->TestFlags(PCF_STMT_START))
               {
                  break;
               }

               if (p->Is(CT_PAREN_OPEN))
               {
                  check_level--;
               }

               if (p->Is(CT_SPAREN_OPEN))
               {
                  break;
               }
               p = p->GetPrevNc(E_Scope::PREPROC);

               if (p->GetLevel() < check_level - 1)
               {
                  break;
               }
            }
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

            if (p->GetParentType() == CT_WHILE)
            {
               continue;
            }

            if (pc->TestFlags(PCF_IN_ENUM))            // Issue #4191
            {
               // look for COMMA or BRACE_CLOSE, what occurs first
               Chunk *found_comma = pc->GetNextType(CT_COMMA, pc->GetLevel());

               if (found_comma->IsNotNullChunk())
               {
                  check_bool_parens(pc, found_comma, 0);
                  pc = found_comma;
               }
               else
               {
                  // look for BRACE_CLOSE
                  Chunk *found_brace = pc->GetNextType(CT_BRACE_CLOSE, pc->GetLevel() - 1);
                  check_bool_parens(pc, found_brace, 0);
                  pc = found_brace;
               }
            }
            else
            {
               // Grab the semicolon, must be the same block
               Chunk *semicolon = pc->GetNextType(CT_SEMICOLON, pc->GetLevel(), E_Scope::PREPROC);

               if (semicolon->IsNotNullChunk())
               {
                  check_bool_parens(pc, semicolon, 0);
                  pc = semicolon;
               }
            }
         }
      }
   }
} // do_parens_assign


void do_parens_return()                         // Issue #3316
{
   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_return_bool");

   if (options::mod_full_paren_return_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while ((pc = pc->GetNextNcNnl())->IsNotNullChunk())
      {
         if (pc->Is(CT_RETURN))
         {
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->Text(), pc->GetLevel());
            // look before for a open sparen
            size_t check_level = pc->GetLevel();
            Chunk  *p          = pc->GetPrevNc(E_Scope::PREPROC);

            while (p->IsNotNullChunk())
            {
               LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                       __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

               //log_pcf_flags(LPARADD, p->GetFlags());
               if (p->TestFlags(PCF_STMT_START))
               {
                  break;
               }

               if (p->Is(CT_PAREN_OPEN))
               {
                  check_level--;
               }

               if (p->Is(CT_SPAREN_OPEN))
               {
                  break;
               }
               p = p->GetPrevNc(E_Scope::PREPROC);

               if (p->GetLevel() < check_level - 1)
               {
                  break;
               }
            }
            LOG_FMT(LPARADD, "%s(%d): orig line is %zu, text is '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, p->GetOrigLine(), p->Text(), p->GetLevel(), get_token_name(p->GetType()));

            if (p->GetParentType() == CT_WHILE)
            {
               continue;
            }
            // Grab the semicolon
            Chunk *semicolon = pc->GetNextType(CT_SEMICOLON, pc->GetLevel(), E_Scope::PREPROC);

            if (semicolon->IsNotNullChunk())
            {
               check_bool_parens(pc, semicolon, 0);
               pc = semicolon;
            }
         }
      }
   }
} // do_parens_return


static void add_parens_between(Chunk *first, Chunk *last)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LPARADD, "%s(%d): first: line %zu, col %zu, between '%s' [lvl is %zu] and\n",
           __func__, __LINE__, first->GetOrigLine(), first->GetOrigCol(),
           first->Text(), first->GetLevel());
   LOG_FMT(LPARADD, "%s(%d): last: line %zu, col %zu, '%s' [lvl is %zu]\n",
           __func__, __LINE__, last->GetOrigLine(), last->GetOrigCol(),
           last->Text(), last->GetLevel());

   // Don't do anything if we have a bad sequence, ie "&& )"
   Chunk *first_n = first->GetNextNcNnl();

   if (first_n == last)
   {
      return;
   }
   Chunk pc;

   pc.SetType(CT_PAREN_OPEN);
   pc.SetOrigLine(first_n->GetOrigLine());
   pc.SetColumn(first_n->GetColumn());                         // Issue #3236
   pc.SetOrigCol(first_n->GetOrigCol());                       // Issue #3236
   pc.SetOrigColEnd(first_n->GetOrigColEnd());                 // Issue #3236
   pc.Str() = "(";
   pc.SetFlags(first_n->GetFlags() & PCF_COPY_FLAGS);
   pc.SetLevel(first_n->GetLevel());
   pc.SetPpLevel(first_n->GetPpLevel());
   pc.SetBraceLevel(first_n->GetBraceLevel());
   pc.CopyAndAddBefore(first_n);

   shift_the_rest_of_the_line(first_n);                         // Issue #3236

   Chunk *last_prev = last->GetPrevNcNnl(E_Scope::PREPROC);

   pc.SetType(CT_PAREN_CLOSE);
   pc.SetOrigLine(last_prev->GetOrigLine());
   pc.SetOrigCol(last_prev->GetOrigCol());
   pc.SetColumn(last_prev->GetColumn() + 1);                         // Issue #3236
   pc.SetOrigCol(last_prev->GetOrigCol() + 1);                       // Issue #3236
   pc.SetOrigColEnd(last_prev->GetOrigColEnd() + 1);                 // Issue #3236
   pc.Str() = ")";
   pc.SetFlags(last_prev->GetFlags() & PCF_COPY_FLAGS);
   pc.SetLevel(last_prev->GetLevel());
   pc.SetPpLevel(last_prev->GetPpLevel());
   pc.SetBraceLevel(last_prev->GetBraceLevel());
   pc.CopyAndAddAfter(last_prev);

   shift_the_rest_of_the_line(last);                         // Issue #3236

   for (Chunk *tmp = first_n;
        tmp != last_prev;
        tmp = tmp->GetNextNcNnl())
   {
      tmp->SetLevel(tmp->GetLevel() + 1);
   }

   last_prev->SetLevel(last_prev->GetLevel() + 1);
} // add_parens_between


static void check_bool_parens(Chunk *popen, Chunk *pclose, int nest)
{
   LOG_FUNC_ENTRY();

   Chunk *ref        = popen;
   bool  hit_compare = false;

   LOG_FMT(LPARADD, "%s(%d): nest is %d, popen on line %zu, orig col is %zu, pclose on line %zu, orig col is %zu, level is %zu\n",
           __func__, __LINE__, nest,
           popen->GetOrigLine(), popen->GetOrigCol(),
           pclose->GetOrigLine(), pclose->GetOrigCol(),
           popen->GetLevel());

   Chunk *pc = popen;

   while (  (pc = pc->GetNextNcNnl())->IsNotNullChunk()
         && pc != pclose)
   {
      if (pc->TestFlags(PCF_IN_PREPROC))
      {
         LOG_FMT(LPARADD2, " -- bail on PP %s [%s] at line %zu col %zu, level %zu\n",
                 get_token_name(pc->GetType()),
                 pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
         return;
      }

      if (  pc->Is(CT_BOOL)
         || pc->Is(CT_QUESTION)
         || pc->Is(CT_COND_COLON)
         || pc->Is(CT_COMMA))
      {
         LOG_FMT(LPARADD2, " -- %s [%s] at line %zu col %zu, level %zu\n",
                 get_token_name(pc->GetType()),
                 pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());

         if (hit_compare)
         {
            hit_compare = false;

            if (!language_is_set(lang_flag_e::LANG_CS))
            {
               add_parens_between(ref, pc);
            }
         }
         ref = pc;
      }
      else if (pc->Is(CT_COMPARE))
      {
         LOG_FMT(LPARADD2, " -- compare '%s' at line %zu, orig col is %zu, level is %zu\n",
                 pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
         hit_compare = true;
      }
      else if (pc->IsParenOpen())
      {
         Chunk *next = pc->GetClosingParen();

         if (next->IsNotNullChunk())
         {
            check_bool_parens(pc, next, nest + 1);
            pc = next;
         }
      }
      else if (pc->Is(CT_SEMICOLON))                      // Issue #3236
      {
         LOG_FMT(LPARADD, "%s(%d): ++++ popen on line %zu, orig col is %zu, level is %zu\n",
                 __func__, __LINE__,
                 popen->GetOrigLine(), popen->GetOrigCol(), popen->GetLevel());
         LOG_FMT(LPARADD, "%s(%d): ++++ pc    on line %zu, orig col is %zu, level is %zu\n",
                 __func__, __LINE__,
                 pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
         ref = pc;
      }
      else if (  pc->Is(CT_BRACE_OPEN)
              || pc->Is(CT_SQUARE_OPEN)
              || pc->Is(CT_ANGLE_OPEN))
      {
         // Skip [], {}, and <>
         pc = pc->GetClosingParen();
      }
   }

   if (  hit_compare
      && ref != popen
      && !language_is_set(lang_flag_e::LANG_CS))
   {
      add_parens_between(ref, pclose);
   }
} // check_bool_parens
