/**
 * @file handle_oc.cpp.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#include "handle_oc.h"

#include "chunk_list.h"
#include "combine_fix.h"
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


bool is_oc_block(chunk_t *pc)
{
   return(  pc != nullptr
         && (  get_chunk_parent_type(pc) == CT_OC_BLOCK_TYPE
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_ARG
            || get_chunk_parent_type(pc) == CT_OC_BLOCK
            || chunk_is_token(pc, CT_OC_BLOCK_CARET)
            || (pc->next != nullptr && pc->next->type == CT_OC_BLOCK_CARET)
            || (pc->prev != nullptr && pc->prev->type == CT_OC_BLOCK_CARET)));
}


void handle_oc_class(chunk_t *pc)
{
   enum class angle_state_e : unsigned int
   {
      NONE  = 0,
      OPEN  = 1, // '<' found
      CLOSE = 2, // '>' found
   };

   LOG_FUNC_ENTRY();
   chunk_t       *tmp;
   bool          hit_scope     = false;
   bool          passed_name   = false; // Did we pass the name of the class and now there can be only protocols, not generics
   int           generic_level = 0;     // level of depth of generic
   angle_state_e as            = angle_state_e::NONE;

   LOG_FMT(LOCCLASS, "%s(%d): start [%s] [%s] line %zu\n",
           __func__, __LINE__, pc->text(), get_token_name(get_chunk_parent_type(pc)), pc->orig_line);

   if (get_chunk_parent_type(pc) == CT_OC_PROTOCOL)
   {
      tmp = chunk_get_next_ncnl(pc);

      if (chunk_is_semicolon(tmp))
      {
         set_chunk_parent(tmp, get_chunk_parent_type(pc));
         LOG_FMT(LOCCLASS, "%s(%d):   bail on semicolon\n", __func__, __LINE__);
         return;
      }
   }
   tmp = pc;

   while ((tmp = chunk_get_next_nnl(tmp)) != nullptr)
   {
      LOG_FMT(LOCCLASS, "%s(%d):       orig_line is %zu, [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->text());

      if (chunk_is_token(tmp, CT_OC_END))
      {
         break;
      }

      if (chunk_is_token(tmp, CT_PAREN_OPEN))
      {
         passed_name = true;
      }

      if (chunk_is_str(tmp, "<", 1))
      {
         set_chunk_type(tmp, CT_ANGLE_OPEN);

         if (passed_name)
         {
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }
         else
         {
            set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);
            generic_level++;
         }
         as = angle_state_e::OPEN;
      }

      if (chunk_is_str(tmp, ">", 1))
      {
         set_chunk_type(tmp, CT_ANGLE_CLOSE);

         if (passed_name)
         {
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
            as = angle_state_e::CLOSE;
         }
         else
         {
            set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);

            if (generic_level == 0)
            {
               fprintf(stderr, "%s(%d): generic_level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            generic_level--;

            if (generic_level == 0)
            {
               as = angle_state_e::CLOSE;
            }
         }
      }

      if (chunk_is_str(tmp, ">>", 2))
      {
         set_chunk_type(tmp, CT_ANGLE_CLOSE);
         set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);
         split_off_angle_close(tmp);
         generic_level -= 1;

         if (generic_level == 0)
         {
            as = angle_state_e::CLOSE;
         }
      }

      if (  chunk_is_token(tmp, CT_BRACE_OPEN)
         && get_chunk_parent_type(tmp) != CT_ASSIGN)
      {
         as = angle_state_e::CLOSE;
         set_chunk_parent(tmp, CT_OC_CLASS);
         tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);

         if (  tmp != nullptr
            && get_chunk_parent_type(tmp) != CT_ASSIGN)
         {
            set_chunk_parent(tmp, CT_OC_CLASS);
         }
      }
      else if (chunk_is_token(tmp, CT_COLON))
      {
         if (as != angle_state_e::OPEN)
         {
            passed_name = true;
         }
         set_chunk_type(tmp, hit_scope ? CT_OC_COLON : CT_CLASS_COLON);

         if (chunk_is_token(tmp, CT_CLASS_COLON))
         {
            set_chunk_parent(tmp, CT_OC_CLASS);
         }
      }
      else if (chunk_is_str(tmp, "-", 1) || chunk_is_str(tmp, "+", 1))
      {
         as = angle_state_e::CLOSE;

         if (chunk_is_newline(chunk_get_prev(tmp)))
         {
            set_chunk_type(tmp, CT_OC_SCOPE);
            chunk_flags_set(tmp, PCF_STMT_START);
            hit_scope = true;
         }
      }

      if (as == angle_state_e::OPEN)
      {
         if (passed_name)
         {
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }
         else
         {
            set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);
         }
      }
   }

   if (chunk_is_token(tmp, CT_BRACE_OPEN))
   {
      tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);

      if (tmp != nullptr)
      {
         set_chunk_parent(tmp, CT_OC_CLASS);
      }
   }
} // handle_oc_class


void handle_oc_block_literal(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279
   chunk_t *next = chunk_get_next_ncnl(pc);

   if (  pc == nullptr
      || prev == nullptr
      || next == nullptr)
   {
      return; // let's be paranoid
   }
   /*
    * block literal: '^ RTYPE ( ARGS ) { }'
    * RTYPE and ARGS are optional
    */
   LOG_FMT(LOCBLK, "%s(%d): block literal @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);

   chunk_t *apo = nullptr; // arg paren open
   chunk_t *bbo = nullptr; // block brace open
   chunk_t *bbc;           // block brace close

   LOG_FMT(LOCBLK, "%s(%d):  + scan", __func__, __LINE__);
   chunk_t *tmp;

   for (tmp = next; tmp; tmp = chunk_get_next_ncnl(tmp))
   {
      /* handle '< protocol >' */
      if (chunk_is_str(tmp, "<", 1))
      {
         chunk_t *ao = tmp;
         chunk_t *ac = chunk_get_next_str(ao, ">", 1, ao->level);

         if (ac)
         {
            set_chunk_type(ao, CT_ANGLE_OPEN);
            set_chunk_parent(ao, CT_OC_PROTO_LIST);
            set_chunk_type(ac, CT_ANGLE_CLOSE);
            set_chunk_parent(ac, CT_OC_PROTO_LIST);

            for (tmp = chunk_get_next(ao); tmp != ac; tmp = chunk_get_next(tmp))
            {
               tmp->level += 1;
               set_chunk_parent(tmp, CT_OC_PROTO_LIST);
            }
         }
         tmp = chunk_get_next_ncnl(ac);
      }
      LOG_FMT(LOCBLK, " '%s'", tmp->text());

      if (tmp->level < pc->level || chunk_is_token(tmp, CT_SEMICOLON))
      {
         LOG_FMT(LOCBLK, "[DONE]");
         break;
      }

      if (tmp->level == pc->level)
      {
         if (chunk_is_paren_open(tmp))
         {
            apo = tmp;
            LOG_FMT(LOCBLK, "[PAREN]");
         }

         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            LOG_FMT(LOCBLK, "[BRACE]");
            bbo = tmp;
            break;
         }
      }
   }

   // make sure we have braces
   bbc = chunk_skip_to_match(bbo);

   if (  bbo == nullptr
      || bbc == nullptr)
   {
      LOG_FMT(LOCBLK, " -- no braces found\n");
      return;
   }
   LOG_FMT(LOCBLK, "\n");

   // we are on a block literal for sure
   set_chunk_type(pc, CT_OC_BLOCK_CARET);
   set_chunk_parent(pc, CT_OC_BLOCK_EXPR);

   // handle the optional args
   chunk_t *lbp; // last before paren - end of return type, if any

   if (apo)
   {
      chunk_t *apc = chunk_skip_to_match(apo);  // arg parenthesis close

      if (chunk_is_paren_close(apc))
      {
         LOG_FMT(LOCBLK, " -- marking parens @ apo->orig_line is %zu, apo->orig_col is %zu and apc->orig_line is %zu, apc->orig_col is %zu\n",
                 apo->orig_line, apo->orig_col, apc->orig_line, apc->orig_col);
         flag_parens(apo, PCF_OC_ATYPE, CT_FPAREN_OPEN, CT_OC_BLOCK_EXPR, true);
         fix_fcn_def_params(apo);
      }
      lbp = chunk_get_prev_ncnlni(apo);   // Issue #2279
   }
   else
   {
      lbp = chunk_get_prev_ncnlni(bbo);   // Issue #2279
   }

   // mark the return type, if any
   while (lbp != pc)
   {
      LOG_FMT(LOCBLK, " -- lbp %s[%s]\n", lbp->text(), get_token_name(lbp->type));
      make_type(lbp);
      chunk_flags_set(lbp, PCF_OC_RTYPE);
      set_chunk_parent(lbp, CT_OC_BLOCK_EXPR);
      lbp = chunk_get_prev_ncnlni(lbp);   // Issue #2279
   }
   // mark the braces
   set_chunk_parent(bbo, CT_OC_BLOCK_EXPR);
   set_chunk_parent(bbc, CT_OC_BLOCK_EXPR);
} // handle_oc_block_literal


void handle_oc_block_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }

   if (pc->flags.test(PCF_IN_TYPEDEF))
   {
      LOG_FMT(LOCBLK, "%s(%d): skip block type @ orig_line is %zu, orig_col is %zu, -- in typedef\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col);
      return;
   }
   // make sure we have '( ^'
   chunk_t *tpo = chunk_get_prev_ncnlni(pc); // type paren open   Issue #2279

   if (chunk_is_paren_open(tpo))
   {
      /*
       * block type: 'RTYPE (^LABEL)(ARGS)'
       * LABEL is optional.
       */
      chunk_t *tpc = chunk_skip_to_match(tpo);   // type close paren (after '^')
      chunk_t *nam = chunk_get_prev_ncnlni(tpc); // name (if any) or '^'   Issue #2279
      chunk_t *apo = chunk_get_next_ncnl(tpc);   // arg open paren
      chunk_t *apc = chunk_skip_to_match(apo);   // arg close paren

      /*
       * If this is a block literal instead of a block type, 'nam'
       * will actually be the closing bracket of the block. We run into
       * this situation if a block literal is enclosed in parentheses.
       */
      if (chunk_is_closing_brace(nam))
      {
         return(handle_oc_block_literal(pc));
      }

      // Check apo is '(' or else this might be a block literal. Issue 2643.
      if (!chunk_is_paren_open(apo))
      {
         return(handle_oc_block_literal(pc));
      }

      if (chunk_is_paren_close(apc))
      {
         chunk_t   *aft = chunk_get_next_ncnl(apc);
         c_token_t pt;

         if (chunk_is_str(nam, "^", 1))
         {
            set_chunk_type(nam, CT_PTR_TYPE);
            pt = CT_FUNC_TYPE;
         }
         else if (  chunk_is_token(aft, CT_ASSIGN)
                 || chunk_is_token(aft, CT_SEMICOLON))
         {
            set_chunk_type(nam, CT_FUNC_VAR);
            pt = CT_FUNC_VAR;
         }
         else
         {
            set_chunk_type(nam, CT_FUNC_TYPE);
            pt = CT_FUNC_TYPE;
         }
         LOG_FMT(LOCBLK, "%s(%d): block type @ orig_line is %zu, orig_col is %zu, text() '%s'[%s]\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, nam->text(), get_token_name(nam->type));
         set_chunk_type(pc, CT_PTR_TYPE);
         set_chunk_parent(pc, pt);  //CT_OC_BLOCK_TYPE;
         set_chunk_type(tpo, CT_TPAREN_OPEN);
         set_chunk_parent(tpo, pt); //CT_OC_BLOCK_TYPE;
         set_chunk_type(tpc, CT_TPAREN_CLOSE);
         set_chunk_parent(tpc, pt); //CT_OC_BLOCK_TYPE;
         set_chunk_type(apo, CT_FPAREN_OPEN);
         set_chunk_parent(apo, CT_FUNC_PROTO);
         set_chunk_type(apc, CT_FPAREN_CLOSE);
         set_chunk_parent(apc, CT_FUNC_PROTO);
         fix_fcn_def_params(apo);
         mark_function_return_type(nam, chunk_get_prev_ncnlni(tpo), pt);   // Issue #2279
      }
   }
} // handle_oc_block_type


chunk_t *handle_oc_md_type(chunk_t *paren_open, c_token_t ptype, pcf_flags_t flags, bool &did_it)
{
   chunk_t *paren_close;

   if (  !chunk_is_paren_open(paren_open)
      || ((paren_close = chunk_skip_to_match(paren_open)) == nullptr))
   {
      did_it = false;
      return(paren_open);
   }
   did_it = true;

   set_chunk_parent(paren_open, ptype);
   chunk_flags_set(paren_open, flags);
   set_chunk_parent(paren_close, ptype);
   chunk_flags_set(paren_close, flags);

   for (chunk_t *cur = chunk_get_next_ncnl(paren_open);
        cur != paren_close;
        cur = chunk_get_next_ncnl(cur))
   {
      LOG_FMT(LOCMSGD, " <%s|%s>", cur->text(), get_token_name(cur->type));
      chunk_flags_set(cur, flags);
      make_type(cur);
   }

   // returning the chunk after the paren close
   return(chunk_get_next_ncnl(paren_close));
}


void handle_oc_message_decl(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   bool did_it;
   //bool      in_paren  = false;
   //int       paren_cnt = 0;
   //int       arg_cnt   = 0;

   // Figure out if this is a spec or decl
   chunk_t *tmp = pc;

   while ((tmp = chunk_get_next(tmp)) != nullptr)
   {
      if (tmp->level < pc->level)
      {
         // should not happen
         return;
      }

      if (chunk_is_token(tmp, CT_SEMICOLON) || chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         break;
      }
   }

   if (tmp == nullptr)
   {
      return;
   }
   c_token_t pt = (tmp->type == CT_SEMICOLON) ? CT_OC_MSG_SPEC : CT_OC_MSG_DECL;

   set_chunk_type(pc, CT_OC_SCOPE);
   set_chunk_parent(pc, pt);

   LOG_FMT(LOCMSGD, "%s(%d): %s @ orig_line is %zu, orig_col is %zu -",
           __func__, __LINE__, get_token_name(pt), pc->orig_line, pc->orig_col);

   // format: -(TYPE) NAME [: (TYPE)NAME

   // handle the return type
   tmp = handle_oc_md_type(chunk_get_next_ncnl(pc), pt, PCF_OC_RTYPE, did_it);

   if (!did_it)
   {
      LOG_FMT(LOCMSGD, " -- missing type parens\n");
      return;
   }

   // expect the method name/label
   if (!chunk_is_token(tmp, CT_WORD))
   {
      LOG_FMT(LOCMSGD, " -- missing method name\n");
      return;
   }  // expect the method name/label

   chunk_t *label = tmp;

   set_chunk_type(tmp, pt);
   set_chunk_parent(tmp, pt);
   pc = chunk_get_next_ncnl(tmp);

   LOG_FMT(LOCMSGD, " [%s]%s", pc->text(), get_token_name(pc->type));

   // if we have a colon next, we have args
   if (chunk_is_token(pc, CT_COLON) || chunk_is_token(pc, CT_OC_COLON))
   {
      pc = label;

      while (true)
      {
         // skip optional label
         if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, pt))
         {
            set_chunk_parent(pc, pt);
            pc = chunk_get_next_ncnl(pc);
         }

         // a colon must be next
         if (!chunk_is_str(pc, ":", 1))
         {
            break;
         }
         set_chunk_type(pc, CT_OC_COLON);
         set_chunk_parent(pc, pt);
         pc = chunk_get_next_ncnl(pc);

         // next is the type in parens
         LOG_FMT(LOCMSGD, "  (%s)", pc->text());
         tmp = handle_oc_md_type(pc, pt, PCF_OC_ATYPE, did_it);

         if (!did_it)
         {
            LOG_FMT(LWARN, "%s(%d): orig_line is %zu, orig_col is %zu expected type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            break;
         }
         // attributes for a method parameter sit between the parameter type and the parameter name
         pc = skip_attribute_next(tmp);
         // we should now be on the arg name
         chunk_flags_set(pc, PCF_VAR_DEF);
         LOG_FMT(LOCMSGD, " arg[%s]", pc->text());
         pc = chunk_get_next_ncnl(pc);
      }
   }
   LOG_FMT(LOCMSGD, " end[%s]", pc->text());

   if (chunk_is_token(pc, CT_BRACE_OPEN))
   {
      set_chunk_parent(pc, pt);
      pc = chunk_skip_to_match(pc);

      if (pc != nullptr)
      {
         set_chunk_parent(pc, pt);
      }
   }
   else if (chunk_is_token(pc, CT_SEMICOLON))
   {
      set_chunk_parent(pc, pt);
   }
   LOG_FMT(LOCMSGD, "\n");
} // handle_oc_message_decl


void handle_oc_message_send(chunk_t *os)
{
   LOG_FUNC_ENTRY();

   chunk_t *cs = chunk_get_next(os);

   while (cs != nullptr && cs->level > os->level)
   {
      cs = chunk_get_next(cs);
   }

   if (cs == nullptr || cs->type != CT_SQUARE_CLOSE)
   {
      return;
   }
   LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, os->orig_line, os->orig_col);

   chunk_t *tmp = chunk_get_next_ncnl(cs);

   if (chunk_is_semicolon(tmp))
   {
      set_chunk_parent(tmp, CT_OC_MSG);
   }
   // expect a word first thing or [...]
   tmp = chunk_get_next_ncnl(os);

   if (  chunk_is_token(tmp, CT_SQUARE_OPEN) || chunk_is_token(tmp, CT_PAREN_OPEN)
      || (chunk_is_token(tmp, CT_OC_AT)))
   {
      chunk_t *tt = chunk_get_next_ncnl(tmp);

      if ((chunk_is_token(tmp, CT_OC_AT)) && tt)
      {
         if (  (chunk_is_token(tt, CT_PAREN_OPEN))
            || (chunk_is_token(tt, CT_BRACE_OPEN))
            || (chunk_is_token(tt, CT_SQUARE_OPEN)))
         {
            tmp = tt;
         }
         else
         {
            LOG_FMT(LOCMSG, "%s(%d): tmp->orig_line is %zu, tmp->orig_col is %zu, expected identifier, not '%s' [%s]\n",
                    __func__, __LINE__, tmp->orig_line, tmp->orig_col,
                    tmp->text(), get_token_name(tmp->type));
            return;
         }
      }
      tmp = chunk_skip_to_match(tmp);
   }
   else if (  tmp->type != CT_WORD
           && tmp->type != CT_TYPE
           && tmp->type != CT_THIS
           && tmp->type != CT_STAR
           && tmp->type != CT_STRING)
   {
      LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu, expected identifier, not '%s' [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col,
              tmp->text(), get_token_name(tmp->type));
      return;
   }
   else
   {
      if (chunk_is_star(tmp)) // Issue #2722
      {
         set_chunk_type(tmp, CT_PTR_TYPE);
         tmp = chunk_get_next_ncnl(tmp);
      }
      chunk_t *tt = chunk_get_next_ncnl(tmp);

      if (chunk_is_paren_open(tt))
      {
         LOG_FMT(LFCN, "%s(%d): (18) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());
         set_chunk_type(tmp, CT_FUNC_CALL);
         tmp = chunk_get_prev_ncnlni(set_paren_parent(tt, CT_FUNC_CALL));   // Issue #2279
      }
      else
      {
         set_chunk_type(tmp, CT_OC_MSG_CLASS);
      }
   }
   set_chunk_parent(os, CT_OC_MSG);
   chunk_flags_set(os, PCF_IN_OC_MSG);
   set_chunk_parent(cs, CT_OC_MSG);
   chunk_flags_set(cs, PCF_IN_OC_MSG);

   // handle '< protocol >'
   tmp = chunk_get_next_ncnl(tmp);

   if (chunk_is_str(tmp, "<", 1))
   {
      chunk_t *ao = tmp;
      chunk_t *ac = chunk_get_next_str(ao, ">", 1, ao->level);

      if (ac)
      {
         set_chunk_type(ao, CT_ANGLE_OPEN);
         set_chunk_parent(ao, CT_OC_PROTO_LIST);
         set_chunk_type(ac, CT_ANGLE_CLOSE);
         set_chunk_parent(ac, CT_OC_PROTO_LIST);

         for (tmp = chunk_get_next(ao); tmp != ac; tmp = chunk_get_next(tmp))
         {
            tmp->level += 1;
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }
      }
      tmp = chunk_get_next_ncnl(ac);
   }
   // handle 'object.property' and 'collection[index]'
   else
   {
      while (tmp)
      {
         if (chunk_is_token(tmp, CT_MEMBER))  // move past [object.prop1.prop2
         {
            chunk_t *typ = chunk_get_next_ncnl(tmp);

            if (chunk_is_token(typ, CT_WORD) || chunk_is_token(typ, CT_TYPE))
            {
               tmp = chunk_get_next_ncnl(typ);
            }
            else
            {
               break;
            }
         }
         else if (chunk_is_token(tmp, CT_SQUARE_OPEN))  // move past [collection[index]
         {
            chunk_t *tcs = chunk_get_next_ncnl(tmp);

            while (tcs != nullptr && tcs->level > tmp->level)
            {
               tcs = chunk_get_next_ncnl(tcs);
            }

            if (chunk_is_token(tcs, CT_SQUARE_CLOSE))
            {
               tmp = chunk_get_next_ncnl(tcs);
            }
            else
            {
               break;
            }
         }
         else
         {
            break;
         }
      }
   }

   // [(self.foo.bar) method]
   if (chunk_is_paren_open(tmp))
   {
      tmp = chunk_get_next_ncnl(chunk_skip_to_match(tmp));
   }

   if (chunk_is_token(tmp, CT_WORD) || chunk_is_token(tmp, CT_TYPE))
   {
      set_chunk_type(tmp, CT_OC_MSG_FUNC);
   }
   chunk_t *prev = nullptr;

   for (tmp = chunk_get_next(os); tmp != cs; tmp = chunk_get_next(tmp))
   {
      chunk_flags_set(tmp, PCF_IN_OC_MSG);

      if (tmp->level == cs->level + 1)
      {
         if (chunk_is_token(tmp, CT_COLON))
         {
            set_chunk_type(tmp, CT_OC_COLON);

            if (chunk_is_token(prev, CT_WORD) || chunk_is_token(prev, CT_TYPE))
            {
               // Might be a named param, check previous block
               chunk_t *pp = chunk_get_prev(prev);

               if (  pp != nullptr
                  && pp->type != CT_OC_COLON
                  && pp->type != CT_ARITH
                  && pp->type != CT_CARET)
               {
                  set_chunk_type(prev, CT_OC_MSG_NAME);
                  set_chunk_parent(tmp, CT_OC_MSG_NAME);
               }
            }
         }
      }
      prev = tmp;
   }
} // handle_oc_message_send


void handle_oc_available(chunk_t *os)
{
   os = chunk_get_next(os);

   while (os != nullptr)
   {
      c_token_t origType = os->type;
      set_chunk_type(os, CT_OC_AVAILABLE_VALUE);

      if (origType == CT_PAREN_CLOSE)
      {
         break;
      }
      os = chunk_get_next(os);
   }
}


void handle_oc_property_decl(chunk_t *os)
{
   log_rule_B("mod_sort_oc_properties");

   if (options::mod_sort_oc_properties())
   {
      typedef std::vector<chunk_t *> ChunkGroup;

      chunk_t                 *next       = chunk_get_next(os);
      chunk_t                 *open_paren = nullptr;

      std::vector<ChunkGroup> class_chunks;       // class
      std::vector<ChunkGroup> thread_chunks;      // atomic, nonatomic
      std::vector<ChunkGroup> readwrite_chunks;   // readwrite, readonly
      std::vector<ChunkGroup> ref_chunks;         // retain, copy, assign, weak, strong, unsafe_unretained
      std::vector<ChunkGroup> getter_chunks;      // getter
      std::vector<ChunkGroup> setter_chunks;      // setter
      std::vector<ChunkGroup> nullability_chunks; // nonnull, nullable, null_unspecified, null_resettable
      std::vector<ChunkGroup> other_chunks;       // any words other than above

      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         open_paren = next;
         next       = chunk_get_next(next);

         /*
          * Determine location of the property attributes
          * NOTE: Did not do this in the combine.cpp do_symbol_check as
          * I was not sure what the ramifications of adding a new type
          * for each of the below types would be. It did break some items
          * when I attempted to add them so this is my hack for now.
          */
         while (next != nullptr && next->type != CT_PAREN_CLOSE)
         {
            if (chunk_is_token(next, CT_OC_PROPERTY_ATTR))
            {
               if (  chunk_is_str(next, "atomic", 6)
                  || chunk_is_str(next, "nonatomic", 9))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  thread_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "readonly", 8)
                       || chunk_is_str(next, "readwrite", 9))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  readwrite_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "assign", 6)
                       || chunk_is_str(next, "retain", 6)
                       || chunk_is_str(next, "copy", 4)
                       || chunk_is_str(next, "strong", 6)
                       || chunk_is_str(next, "weak", 4)
                       || chunk_is_str(next, "unsafe_unretained", 17))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  ref_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "getter", 6))
               {
                  ChunkGroup chunkGroup;

                  do
                  {
                     chunkGroup.push_back(next);
                     next = chunk_get_next(next);
                  } while (  next
                          && next->type != CT_COMMA
                          && next->type != CT_PAREN_CLOSE);

                  next = next->prev;

                  // coverity CID 160946
                  if (next == nullptr)
                  {
                     break;
                  }
                  getter_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "setter", 6))
               {
                  ChunkGroup chunkGroup;

                  do
                  {
                     chunkGroup.push_back(next);
                     next = chunk_get_next(next);
                  } while (  next
                          && next->type != CT_COMMA
                          && next->type != CT_PAREN_CLOSE);

                  next = chunk_get_prev(next);

                  if (next == nullptr)
                  {
                     break;
                  }
                  setter_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "nullable", 8)
                       || chunk_is_str(next, "nonnull", 7)
                       || chunk_is_str(next, "null_resettable", 15)
                       || chunk_is_str(next, "null_unspecified", 16))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  nullability_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "class", 5))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  class_chunks.push_back(chunkGroup);
               }
               else
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  other_chunks.push_back(chunkGroup);
               }
            }
            else if (chunk_is_word(next))
            {
               if (chunk_is_str(next, "class", 5))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  class_chunks.push_back(chunkGroup);
               }
               else
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  other_chunks.push_back(chunkGroup);
               }
            }
            next = chunk_get_next(next);
         }
         log_rule_B("mod_sort_oc_property_class_weight");
         int class_w = options::mod_sort_oc_property_class_weight();
         log_rule_B("mod_sort_oc_property_thread_safe_weight");
         int thread_w = options::mod_sort_oc_property_thread_safe_weight();
         log_rule_B("mod_sort_oc_property_readwrite_weight");
         int readwrite_w = options::mod_sort_oc_property_readwrite_weight();
         log_rule_B("mod_sort_oc_property_reference_weight");
         int ref_w = options::mod_sort_oc_property_reference_weight();
         log_rule_B("mod_sort_oc_property_getter_weight");
         int getter_w = options::mod_sort_oc_property_getter_weight();
         log_rule_B("mod_sort_oc_property_setter_weight");
         int setter_w = options::mod_sort_oc_property_setter_weight();
         log_rule_B("mod_sort_oc_property_nullability_weight");
         int nullability_w = options::mod_sort_oc_property_nullability_weight();

         //
         std::multimap<int, std::vector<ChunkGroup> > sorted_chunk_map;
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(class_w, class_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(thread_w, thread_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(readwrite_w, readwrite_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(ref_w, ref_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(getter_w, getter_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(setter_w, setter_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(nullability_w, nullability_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(std::numeric_limits<int>::min(), other_chunks));

         chunk_t *curr_chunk = open_paren;

         for (multimap<int, std::vector<ChunkGroup> >::reverse_iterator it = sorted_chunk_map.rbegin(); it != sorted_chunk_map.rend(); ++it)
         {
            std::vector<ChunkGroup> chunk_groups = (*it).second;

            for (auto chunk_group : chunk_groups)
            {
               for (auto chunk : chunk_group)
               {
                  chunk->orig_prev_sp = 0;

                  if (chunk != curr_chunk)
                  {
                     chunk_move_after(chunk, curr_chunk);
                     curr_chunk = chunk;
                  }
                  else
                  {
                     curr_chunk = chunk_get_next(curr_chunk);
                  }
               }

               // add the parenthesis
               chunk_t endchunk;
               set_chunk_type(&endchunk, CT_COMMA);
               set_chunk_parent(&endchunk, get_chunk_parent_type(curr_chunk));
               endchunk.str         = ",";
               endchunk.level       = curr_chunk->level;
               endchunk.brace_level = curr_chunk->brace_level;
               endchunk.orig_line   = curr_chunk->orig_line;
               endchunk.orig_col    = curr_chunk->orig_col;
               endchunk.column      = curr_chunk->orig_col_end + 1;
               endchunk.flags       = curr_chunk->flags & PCF_COPY_FLAGS;
               chunk_add_after(&endchunk, curr_chunk);
               curr_chunk = curr_chunk->next;
            }
         }

         // Remove the extra comma's that we did not move
         while (curr_chunk && curr_chunk->type != CT_PAREN_CLOSE)
         {
            chunk_t *rm_chunk = curr_chunk;
            curr_chunk = chunk_get_next(curr_chunk);
            chunk_del(rm_chunk);
         }
      }
   }
   chunk_t *tmp = chunk_get_next_ncnl(os);

   if (chunk_is_paren_open(tmp))
   {
      tmp = chunk_get_next_ncnl(chunk_skip_to_match(tmp));
   }
   fix_variable_definition(tmp);
} // handle_oc_property_decl
