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
   LOG_FMT(logsev, "%s Parse Frames(%d):", txt, cpd.frame_count);
   for (int idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, " [%s-%d]",
              get_token_name(cpd.frames[idx].in_ifdef),
              cpd.frames[idx].ref_no);
   }
   LOG_FMT(logsev, "-[%s-%d]\n", get_token_name(pf->in_ifdef), pf->ref_no);
}


static void pf_log_all(log_sev_t logsev)
{
   LOG_FMT(logsev, "##=- Parse Frame : %d entries\n", cpd.frame_count);

   for (int idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, "##  <%d> ", idx);

      pf_log(logsev, &cpd.frames[idx]);
   }
   LOG_FMT(logsev, "##=-\n");
}


void pf_copy(parse_frame_t *dst, const parse_frame_t *src)
{
   memcpy(dst, src, sizeof(parse_frame_t));
}


void pf_push(parse_frame_t *pf)
{
   static int ref_no = 1;

   if (cpd.frame_count < static_cast<int> ARRAY_SIZE(cpd.frames))
   {
      pf_copy(&cpd.frames[cpd.frame_count], pf);
      cpd.frame_count++;

      pf->ref_no = ref_no++;
   }
   LOG_FMT(LPF, "%s(%d): count = %d\n", __func__, __LINE__, cpd.frame_count);
}


void pf_push_under(parse_frame_t *pf)
{
   LOG_FMT(LPF, "%s(%d): before count = %d\n", __func__, __LINE__, cpd.frame_count);

   if (  cpd.frame_count < static_cast<int> ARRAY_SIZE(cpd.frames)
      && cpd.frame_count >= 1)
   {
      parse_frame_t *npf1 = &cpd.frames[cpd.frame_count - 1];
      parse_frame_t *npf2 = &cpd.frames[cpd.frame_count];
      pf_copy(npf2, npf1);
      pf_copy(npf1, pf);
      cpd.frame_count++;
   }

   LOG_FMT(LPF, "%s(%d): after count = %d\n", __func__, __LINE__, cpd.frame_count);
}


void pf_copy_tos(parse_frame_t *pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy(pf, &cpd.frames[cpd.frame_count - 1]);
   }
   LOG_FMT(LPF, "%s(%d): count = %d\n", __func__, __LINE__, cpd.frame_count);
}


void pf_copy_2nd_tos(parse_frame_t *pf)
{
   if (cpd.frame_count > 1)
   {
      pf_copy(pf, &cpd.frames[cpd.frame_count - 2]);
   }
   LOG_FMT(LPF, "%s(%d): count = %d\n", __func__, __LINE__, cpd.frame_count);
}


void pf_trash_tos(void)
{
   if (cpd.frame_count > 0)
   {
      cpd.frame_count--;
   }
   LOG_FMT(LPF, "%s(%d): count = %d\n", __func__, __LINE__, cpd.frame_count);
}


void pf_pop(parse_frame_t *pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy_tos(pf);
      pf_trash_tos();
   }
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
      LOG_FMT(LNOTE, "%s(%d): Preproc parent not set correctly on line %zu: got %s expected %s\n",
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
      LOG_FMT(LPF, "%s(%d): %zu> %s: %s in_ifdef=%d/%d counts=%d/%d\n", __func__, __LINE__,
              pc->orig_line, get_token_name(pc->parent_type), txt,
              in_ifdef, frm->in_ifdef, b4_cnt, cpd.frame_count);
      pf_log_all(LPF);
      LOG_FMT(LPF, " <Out>");
      pf_log(LPF, frm);
   }

   pf_log_frms(LPFCHK, "END", frm);

   return(pp_level);
} // pf_check
