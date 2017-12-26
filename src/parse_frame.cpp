/**
 * @file parse_frame.cpp
 * Does the parse frame stuff, which is used to handle #ifdef stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "parse_frame.h"
#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "uncrustify.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>


//TODO temp conversion functions, remove after transition to ParseFrame
static parse_frame_t genOldFrame(const ParseFrame &frm)
{
   assert(frm.tos() < PSE_SIZE);
   parse_frame_t ret{};

   ret.ref_no       = frm.ref_no;
   ret.level        = frm.level;
   ret.brace_level  = frm.brace_level;
   ret.pp_level     = frm.pp_level;
   ret.sparen_count = frm.sparen_count;
   ret.paren_count  = frm.paren_count;
   ret.in_ifdef     = frm.in_ifdef;
   ret.stmt_count   = frm.stmt_count;
   ret.expr_count   = frm.expr_count;
   ret.pse_tos      = frm.tos();

   for (size_t i = 0; i <= frm.tos(); ++i)
   {
      auto &frm_pse = frm.at(i);

      ret.pse[i].type         = frm_pse.type;
      ret.pse[i].level        = frm_pse.level;
      ret.pse[i].open_line    = frm_pse.open_line;
      ret.pse[i].pc           = frm_pse.pc;
      ret.pse[i].brace_indent = frm_pse.brace_indent;
      ret.pse[i].indent       = frm_pse.indent;
      ret.pse[i].indent_tmp   = frm_pse.indent_tmp;
      ret.pse[i].indent_tab   = frm_pse.indent_tab;
      ret.pse[i].indent_cont  = frm_pse.indent_cont;
      ret.pse[i].parent       = frm_pse.parent;
      ret.pse[i].stage        = frm_pse.stage;
      ret.pse[i].in_preproc   = frm_pse.in_preproc;
      ret.pse[i].ns_cnt       = frm_pse.ns_cnt;
      ret.pse[i].non_vardef   = frm_pse.non_vardef;
      ret.pse[i].ip           = frm_pse.ip;
   }
   //+1 for the out of bounds elem
   ret.pse[frm.tos() + 1] = frm.poped();

   return(ret);
}


static ParseFrame genNewFrame(const parse_frame_t &frm)
{
   ParseFrame ret{};

   ret.ref_no       = frm.ref_no;
   ret.level        = frm.level;
   ret.brace_level  = frm.brace_level;
   ret.pp_level     = frm.pp_level;
   ret.sparen_count = frm.sparen_count;
   ret.paren_count  = frm.paren_count;
   ret.in_ifdef     = frm.in_ifdef;
   ret.stmt_count   = frm.stmt_count;
   ret.expr_count   = frm.expr_count;

   //+1 for the out of bounds elem
   for (size_t i = 1; i <= frm.pse_tos + 1; ++i)
   {
      chunk_t c{};
      ret.push(c);
   }
   for (size_t i = 0; i <= frm.pse_tos + 1; ++i)
   {
      ret.at(i).type         = frm.pse[i].type;
      ret.at(i).level        = frm.pse[i].level;
      ret.at(i).open_line    = frm.pse[i].open_line;
      ret.at(i).pc           = frm.pse[i].pc;
      ret.at(i).brace_indent = frm.pse[i].brace_indent;
      ret.at(i).indent       = frm.pse[i].indent;
      ret.at(i).indent_tmp   = frm.pse[i].indent_tmp;
      ret.at(i).indent_tab   = frm.pse[i].indent_tab;
      ret.at(i).indent_cont  = frm.pse[i].indent_cont;
      ret.at(i).parent       = frm.pse[i].parent;
      ret.at(i).stage        = frm.pse[i].stage;
      ret.at(i).in_preproc   = frm.pse[i].in_preproc;
      ret.at(i).ns_cnt       = frm.pse[i].ns_cnt;
      ret.at(i).non_vardef   = frm.pse[i].non_vardef;
      ret.at(i).ip           = frm.pse[i].ip;
   }
   ret.pop();

   return(ret);
} // genNewFrame


static void pf_log_frms(log_sev_t logsev, const char *txt, parse_frame_t *pf);


//! Logs the entire parse frame stack
static void pf_log_all(log_sev_t logsev);


//! Logs one parse frame
static void pf_log(log_sev_t logsev, parse_frame_t *pf)
{
   LOG_FMT(logsev, "[%s] BrLevel=%d Level=%d PseTos=%zu\n",
           get_token_name(pf->in_ifdef),
           pf->brace_level, pf->level, pf->pse_tos);

   LOG_FMT(logsev, " *");
   for (size_t idx = 1; idx <= pf->pse_tos; idx++)
   {
      LOG_FMT(logsev, " [%s-%d]",
              get_token_name(pf->pse[idx].type),
              (int)pf->pse[idx].stage);
   }
   LOG_FMT(logsev, "\n");
}


static void pf_log_frms(log_sev_t logsev, const char *txt, parse_frame_t *pf)
{
   LOG_FMT(logsev, "%s Parse Frames(%zu):", txt, cpd.frame_count);
   for (size_t idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, " [%s-%d]",
              get_token_name(cpd.frames[idx].in_ifdef),
              cpd.frames[idx].ref_no);
   }
   LOG_FMT(logsev, "-[%s-%d]\n", get_token_name(pf->in_ifdef), pf->ref_no);
}


static void pf_log_all(log_sev_t logsev)
{
   LOG_FMT(logsev, "##=- Parse Frame : %zu entries\n", cpd.frame_count);

   for (size_t idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, "##  idx is %zu, ", idx);

      pf_log(logsev, &cpd.frames[idx]);
   }
   LOG_FMT(logsev, "##=-\n");
}


void pf_copy(parse_frame_t *dst, const parse_frame_t *src)
{
   memcpy(dst, src, sizeof(parse_frame_t));
}


void controlFrameCount()
{
   if (cpd.frame_count == FRAME_SIZE)
   {
      fprintf(stderr, "Number of 'frame' is too big for the current value %d,\n", FRAME_SIZE);
      //fprintf(stderr, "at line %zu, column %zu.\n", pc->orig_line, pc->orig_col);
      fprintf(stderr, "Please make a report.\n");
      log_flush(true);
      exit(EX_SOFTWARE);
   }
}


void pf_push(parse_frame_t *pf)
{
   static int ref_no = 1;

   pf_copy(&cpd.frames[cpd.frame_count], pf);
   cpd.frame_count++;
   controlFrameCount();
   pf->ref_no = ref_no++;
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_push_under(parse_frame_t *pf)
{
   LOG_FMT(LPF, "%s(%d): before frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);

   if (cpd.frame_count >= 1)
   {
      parse_frame_t *npf1 = &cpd.frames[cpd.frame_count - 1];
      parse_frame_t *npf2 = &cpd.frames[cpd.frame_count];
      pf_copy(npf2, npf1);
      pf_copy(npf1, pf);
      cpd.frame_count++;
      controlFrameCount();
   }

   LOG_FMT(LPF, "%s(%d): after frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_copy_tos(parse_frame_t *pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy(pf, &cpd.frames[cpd.frame_count - 1]);
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_copy_2nd_tos(parse_frame_t *pf)
{
   if (cpd.frame_count > 1)
   {
      pf_copy(pf, &cpd.frames[cpd.frame_count - 2]);
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_trash_tos(void)
{
   if (cpd.frame_count > 0)
   {
      cpd.frame_count--;
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_pop(parse_frame_t *pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy_tos(pf);
      pf_trash_tos();
   }
}


int pf_check(ParseFrame &frm, chunk_t *pc)
{
   parse_frame_t o_f = genOldFrame(frm);

   const int     ret = pf_check(&o_f, pc);

   frm = genNewFrame(o_f);

   return(ret);
}


int pf_check(parse_frame_t *frm, chunk_t *pc)
{
   int in_ifdef = frm->in_ifdef;
   int b4_cnt   = cpd.frame_count;
   int pp_level = cpd.pp_level;

   if (pc->type != CT_PREPROC)
   {
      return(pp_level);
   }
   chunk_t *next = chunk_get_next(pc);
   if (next == nullptr)
   {
      return(pp_level);
   }

   if (pc->parent_type != next->type)
   {
      LOG_FMT(LNOTE, "%s(%d): Preproc parent not set correctly on orig_line %zu: got %s expected %s\n",
              __func__, __LINE__, pc->orig_line, get_token_name(pc->parent_type),
              get_token_name(next->type));
      set_chunk_parent(pc, next->type);
   }

   LOG_FMT(LPFCHK, "%s(%d): %zu] %s\n",
           __func__, __LINE__, pc->orig_line, get_token_name(pc->parent_type));
   pf_log_frms(LPFCHK, "TOP", frm);

   const char *txt = nullptr;
   if (pc->flags & PCF_IN_PREPROC)
   {
      LOG_FMT(LPF, " <In> ");
      pf_log(LPF, frm);

      if (pc->parent_type == CT_PP_IF)
      {
         // An #if pushes a copy of the current frame on the stack
         cpd.pp_level++;
         pf_push(frm);
         frm->in_ifdef = CT_PP_IF;
         txt           = "if-push";
      }
      else if (pc->parent_type == CT_PP_ELSE)
      {
         pp_level--;

         /*
          * For #else of #elif, we want to keep the #if part and throw out the
          * else parts.
          * We check to see what the top type is to see if we just push or
          * pop and then push.
          * We need to use the copy right before the if.
          */
         if (frm->in_ifdef == CT_PP_IF)
         {
            // we have [...] [base]-[if], so push an [else]
            pf_push(frm);
            frm->in_ifdef = CT_PP_ELSE;
         }
         // we have [...] [base] [if]-[else], copy [base] over [else]
         pf_copy_2nd_tos(frm);
         frm->in_ifdef = CT_PP_ELSE;
         txt           = "else-push";
      }
      else if (pc->parent_type == CT_PP_ENDIF)
      {
         /*
          * we may have [...] [base] [if]-[else] or [...] [base]-[if].
          * Throw out the [else].
          */
         cpd.pp_level--;
         pp_level--;

         if (frm->in_ifdef == CT_PP_ELSE)
         {
            /*
             * We have: [...] [base] [if]-[else]
             * We want: [...]-[if]
             */
            pf_copy_tos(frm);     // [...] [base] [if]-[if]

            if (cpd.frame_count < 2)
            {
               fprintf(stderr, "Number of 'frame' is too small.\n");
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            frm->in_ifdef = cpd.frames[cpd.frame_count - 2].in_ifdef;
            pf_trash_tos();       // [...] [base]-[if]
            pf_trash_tos();       // [...]-[if]

            txt = "endif-trash/pop";
         }
         else if (frm->in_ifdef == CT_PP_IF)
         {
            /*
             * We have: [...] [base] [if]
             * We want: [...] [base]
             */
            pf_pop(frm);
            txt = "endif-pop";
         }
         else
         {
            txt = "???";
         }
      }
   }

   if (txt != nullptr)
   {
      LOG_FMT(LPF, "%s(%d): orig_line is %zu, type is %s: %s in_ifdef is %d/%d, counts is %d, frame_count is %zu\n",
              __func__, __LINE__,
              pc->orig_line, get_token_name(pc->parent_type), txt,
              in_ifdef, frm->in_ifdef, b4_cnt, cpd.frame_count);
      pf_log_all(LPF);
      LOG_FMT(LPF, " <Out>");
      pf_log(LPF, frm);
   }

   pf_log_frms(LPFCHK, "END", frm);

   return(pp_level);
} // pf_check
