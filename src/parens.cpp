/**
 * @file parens.cpp
 * Adds or removes parens.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "parens.h"

#include "log_rules.h"

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


void do_parens(void)
{
   constexpr static auto LCURRENT = LPARADD;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_if_bool");

   if (options::mod_full_paren_if_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while (  (pc = pc->GetNextNcNnl()) != nullptr
            && pc->IsNotNullChunk())
      {
         if (  pc->type != CT_SPAREN_OPEN
            || (  get_chunk_parent_type(pc) != CT_IF
               && get_chunk_parent_type(pc) != CT_ELSEIF
               && get_chunk_parent_type(pc) != CT_SWITCH))
         {
            continue;
         }
         // Grab the close sparen
         Chunk *pclose = pc->GetNextType(CT_SPAREN_CLOSE, pc->level, E_Scope::PREPROC);

         if (pclose->IsNotNullChunk())
         {
            check_bool_parens(pc, pclose, 0);
            pc = pclose;
         }
      }
   }
} // do_parens


void do_parens_assign(void)                         // Issue #3316
{
   constexpr static auto LCURRENT = LPARADD;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_assign_bool");

   if (options::mod_full_paren_assign_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while (  (pc = pc->GetNextNcNnl()) != nullptr
            && pc->IsNotNullChunk())
      {
         if (chunk_is_token(pc, CT_ASSIGN))
         {
            LOG_FMT(LPARADD, "%s(%d): orig_line is %zu, text is '%s', level is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->Text(), pc->level);
            // look before for a open sparen
            size_t check_level = pc->level;
            Chunk  *p          = pc->GetPrevNc(E_Scope::PREPROC);

            while (p->IsNotNullChunk())
            {
               LOG_FMT(LPARADD, "%s(%d): orig_line is %zu, text is '%s', level is %zu, type is %s\n",
                       __func__, __LINE__, p->orig_line, p->Text(), p->level, get_token_name(p->type));

               //log_pcf_flags(LPARADD, p->flags);
               if (p->flags.test(PCF_STMT_START))
               {
                  break;
               }

               if (chunk_is_token(p, CT_PAREN_OPEN))
               {
                  check_level--;
               }

               if (chunk_is_token(p, CT_SPAREN_OPEN))
               {
                  break;
               }
               p = p->GetPrevNc(E_Scope::PREPROC);

               if (p->level < check_level - 1)
               {
                  break;
               }
            }
            LOG_FMT(LPARADD, "%s(%d): orig_line is %zu, text is '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, p->orig_line, p->Text(), p->level, get_token_name(p->type));

            if (get_chunk_parent_type(p) == CT_WHILE)
            {
               continue;
            }
            // Grab the semicolon
            Chunk *semicolon = pc->GetNextType(CT_SEMICOLON, pc->level, E_Scope::PREPROC);

            if (semicolon->IsNotNullChunk())
            {
               check_bool_parens(pc, semicolon, 0);
               pc = semicolon;
            }
         }
      }
   }
} // do_parens_assign


void do_parens_return(void)                         // Issue #3316
{
   constexpr static auto LCURRENT = LPARADD;

   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_paren_return_bool");

   if (options::mod_full_paren_return_bool())
   {
      Chunk *pc = Chunk::GetHead();

      while (  (pc = pc->GetNextNcNnl()) != nullptr
            && pc->IsNotNullChunk())
      {
         if (chunk_is_token(pc, CT_RETURN))
         {
            LOG_FMT(LPARADD, "%s(%d): orig_line is %zu, text is '%s', level is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->Text(), pc->level);
            // look before for a open sparen
            size_t check_level = pc->level;
            Chunk  *p          = pc->GetPrevNc(E_Scope::PREPROC);

            while (p->IsNotNullChunk())
            {
               LOG_FMT(LPARADD, "%s(%d): orig_line is %zu, text is '%s', level is %zu, type is %s\n",
                       __func__, __LINE__, p->orig_line, p->Text(), p->level, get_token_name(p->type));

               //log_pcf_flags(LPARADD, p->flags);
               if (p->flags.test(PCF_STMT_START))
               {
                  break;
               }

               if (chunk_is_token(p, CT_PAREN_OPEN))
               {
                  check_level--;
               }

               if (chunk_is_token(p, CT_SPAREN_OPEN))
               {
                  break;
               }
               p = p->GetPrevNc(E_Scope::PREPROC);

               if (p->level < check_level - 1)
               {
                  break;
               }
            }
            LOG_FMT(LPARADD, "%s(%d): orig_line is %zu, text is '%s', level is %zu, type is %s\n",
                    __func__, __LINE__, p->orig_line, p->Text(), p->level, get_token_name(p->type));

            if (get_chunk_parent_type(p) == CT_WHILE)
            {
               continue;
            }
            // Grab the semicolon
            Chunk *semicolon = pc->GetNextType(CT_SEMICOLON, pc->level, E_Scope::PREPROC);

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

   LOG_FMT(LPARADD, "%s(%d): line %zu, between '%s' [lvl is %zu] and '%s' [lvl is %zu]\n",
           __func__, __LINE__, first->orig_line,
           first->Text(), first->level,
           last->Text(), last->level);

   // Don't do anything if we have a bad sequence, ie "&& )"
   Chunk *first_n = first->GetNextNcNnl();

   if (first_n == last)
   {
      return;
   }
   Chunk pc;

   set_chunk_type(&pc, CT_PAREN_OPEN);
   pc.orig_line   = first_n->orig_line;
   pc.orig_col    = first_n->orig_col;
   pc.str         = "(";
   pc.flags       = first_n->flags & PCF_COPY_FLAGS;
   pc.level       = first_n->level;
   pc.pp_level    = first_n->pp_level;
   pc.brace_level = first_n->brace_level;

   chunk_add_before(&pc, first_n);

   Chunk *last_p = last->GetPrevNcNnl(E_Scope::PREPROC);

   set_chunk_type(&pc, CT_PAREN_CLOSE);
   pc.orig_line   = last_p->orig_line;
   pc.orig_col    = last_p->orig_col;
   pc.str         = ")";
   pc.flags       = last_p->flags & PCF_COPY_FLAGS;
   pc.level       = last_p->level;
   pc.pp_level    = last_p->pp_level;
   pc.brace_level = last_p->brace_level;

   chunk_add_after(&pc, last_p);

   for (Chunk *tmp = first_n;
        tmp != last_p;
        tmp = tmp->GetNextNcNnl())
   {
      tmp->level++;
   }

   last_p->level++;
} // add_parens_between


static void check_bool_parens(Chunk *popen, Chunk *pclose, int nest)
{
   LOG_FUNC_ENTRY();

   Chunk *ref        = popen;
   bool  hit_compare = false;

   LOG_FMT(LPARADD, "%s(%d): nest is %d, popen on line %zu, orig_col is %zu, pclose on line %zu, orig_col is %zu, level is %zu\n",
           __func__, __LINE__, nest,
           popen->orig_line, popen->orig_col,
           pclose->orig_line, pclose->orig_col,
           popen->level);

   Chunk *pc = popen;

   while (  (pc = pc->GetNextNcNnl()) != nullptr
         && pc->IsNotNullChunk()
         && pc != pclose)
   {
      if (pc->flags.test(PCF_IN_PREPROC))
      {
         LOG_FMT(LPARADD2, " -- bail on PP %s [%s] at line %zu col %zu, level %zu\n",
                 get_token_name(pc->type),
                 pc->Text(), pc->orig_line, pc->orig_col, pc->level);
         return;
      }

      if (  chunk_is_token(pc, CT_BOOL)
         || chunk_is_token(pc, CT_QUESTION)
         || chunk_is_token(pc, CT_COND_COLON)
         || chunk_is_token(pc, CT_COMMA))
      {
         LOG_FMT(LPARADD2, " -- %s [%s] at line %zu col %zu, level %zu\n",
                 get_token_name(pc->type),
                 pc->Text(), pc->orig_line, pc->orig_col, pc->level);

         if (hit_compare)
         {
            hit_compare = false;

            if (!language_is_set(LANG_CS))
            {
               add_parens_between(ref, pc);
            }
         }
         ref = pc;
      }
      else if (chunk_is_token(pc, CT_COMPARE))
      {
         LOG_FMT(LPARADD2, " -- compare '%s' at line %zu, orig_col is %zu, level is %zu\n",
                 pc->Text(), pc->orig_line, pc->orig_col, pc->level);
         hit_compare = true;
      }
      else if (chunk_is_paren_open(pc))
      {
         Chunk *next = chunk_skip_to_match(pc);

         if (next != nullptr)
         {
            check_bool_parens(pc, next, nest + 1);
            pc = next;
         }
      }
      else if (  chunk_is_token(pc, CT_BRACE_OPEN)
              || chunk_is_token(pc, CT_SQUARE_OPEN)
              || chunk_is_token(pc, CT_ANGLE_OPEN))
      {
         // Skip [], {}, and <>
         pc = chunk_skip_to_match(pc);
      }
   }

   if (  hit_compare
      && ref != popen
      && !language_is_set(LANG_CS))
   {
      add_parens_between(ref, pclose);
   }
} // check_bool_parens
