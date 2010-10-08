/**
 * @file parse_frame.cpp
 * Does the parse frame stuff, which is used to handle #ifdef stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

/**
 * Logs one parse frame
 */
void pf_log(int logsev, struct parse_frame *pf)
{
   int idx;

   LOG_FMT(logsev, "[%s] BrLevel=%d Level=%d PseTos=%d\n",
           get_token_name(pf->in_ifdef),
           pf->brace_level, pf->level, pf->pse_tos);

   LOG_FMT(logsev, " *");
   for (idx = 1; idx <= pf->pse_tos; idx++)
   {
      LOG_FMT(logsev, " [%s-%d]",
              get_token_name(pf->pse[idx].type),
              pf->pse[idx].stage);
   }
   LOG_FMT(logsev, "\n");
}


static void pf_log_frms(int logsev, const char *txt, struct parse_frame *pf)
{
   int idx;

   LOG_FMT(logsev, "%s Parse Frames(%d):", txt, cpd.frame_count);
   for (idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, " [%s-%d]",
              get_token_name(cpd.frames[idx].in_ifdef),
              cpd.frames[idx].ref_no);
   }
   LOG_FMT(logsev, "-[%s-%d]\n", get_token_name(pf->in_ifdef), pf->ref_no);
}


/**
 * Logs the entire parse frame stack
 */
void pf_log_all(int logsev)
{
   int idx;

   LOG_FMT(logsev, "##=- Parse Frame : %d entries\n", cpd.frame_count);

   for (idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, "##  <%d> ", idx);

      pf_log(logsev, &cpd.frames[idx]);
   }
   LOG_FMT(logsev, "##=-\n");
}


/**
 * Copies src to dst.
 */
void pf_copy(struct parse_frame *dst, const struct parse_frame *src)
{
   memcpy(dst, src, sizeof(struct parse_frame));
}


/**
 * Push a copy of the parse frame onto the stack.
 * This is called on #if and #ifdef.
 */
void pf_push(struct parse_frame *pf)
{
   static int ref_no = 1;

   if (cpd.frame_count < (int)ARRAY_SIZE(cpd.frames))
   {
      pf_copy(&cpd.frames[cpd.frame_count], pf);
      cpd.frame_count++;
      pf->ref_no = ref_no++;
   }
   LOG_FMT(LPF, "%s: count = %d\n", __func__, cpd.frame_count);
}


/**
 * Push a copy of the parse frame onto the stack, under the tos.
 * If this were a linked list, just add before the last item.
 * This is called on the first #else and #elif.
 */
void pf_push_under(struct parse_frame *pf)
{
   struct parse_frame *npf1;
   struct parse_frame *npf2;

   LOG_FMT(LPF, "%s: before count = %d\n", __func__, cpd.frame_count);

   if ((cpd.frame_count < (int)ARRAY_SIZE(cpd.frames)) &&
       (cpd.frame_count >= 1))
   {
      npf1 = &cpd.frames[cpd.frame_count - 1];
      npf2 = &cpd.frames[cpd.frame_count];
      pf_copy(npf2, npf1);
      pf_copy(npf1, pf);
      cpd.frame_count++;
   }

   LOG_FMT(LPF, "%s: after count = %d\n", __func__, cpd.frame_count);
}


/**
 * Copy the top item off the stack into pf.
 * This is called on #else and #elif.
 */
void pf_copy_tos(struct parse_frame *pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy(pf, &cpd.frames[cpd.frame_count - 1]);
   }
   LOG_FMT(LPF, "%s: count = %d\n", __func__, cpd.frame_count);
}


/**
 * Copy the 2nd top item off the stack into pf.
 * This is called on #else and #elif.
 * The stack contains [...] [base] [if] at this point.
 * We want to copy [base].
 */
static void pf_copy_2nd_tos(struct parse_frame *pf)
{
   if (cpd.frame_count > 1)
   {
      pf_copy(pf, &cpd.frames[cpd.frame_count - 2]);
   }
   LOG_FMT(LPF, "%s: count = %d\n", __func__, cpd.frame_count);
}


/**
 * Deletes the top frame from the stack.
 */
void pf_trash_tos(void)
{
   if (cpd.frame_count > 0)
   {
      cpd.frame_count--;
   }
   LOG_FMT(LPF, "%s: count = %d\n", __func__, cpd.frame_count);
}


/**
 * Pop the top item off the stack and copy into pf.
 * This is called on #endif
 */
void pf_pop(struct parse_frame *pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy_tos(pf);
      pf_trash_tos();
   }
   //fprintf(stderr, "%s: count = %d\n", __func__, cpd.frame_count);
}


/**
 * Returns the pp_indent to use for this line
 */
int pf_check(struct parse_frame *frm, chunk_t *pc)
{
   int        in_ifdef = frm->in_ifdef;
   int        b4_cnt   = cpd.frame_count;
   int        pp_level = cpd.pp_level;
   const char *txt     = NULL;
   chunk_t    *next;

   if (pc->type != CT_PREPROC)
   {
      return(pp_level);
   }
   next = chunk_get_next(pc);

   if (pc->parent_type != next->type)
   {
      LOG_FMT(LNOTE, "%s: Preproc parent not set correctly on line %d: got %s expected %s\n",
              __func__, pc->orig_line, get_token_name(pc->parent_type),
              get_token_name(next->type));
      pc->parent_type = next->type;
   }

   LOG_FMT(LPFCHK, "%s: %5d] %s\n",
           __func__, pc->orig_line, get_token_name(pc->parent_type));
   pf_log_frms(LPFCHK, "TOP", frm);

   if ((pc->flags & PCF_IN_PREPROC) != 0)
   {
      LOG_FMT(LPF, " <In> ");
      pf_log(LPF, frm);

      if (pc->parent_type == CT_PP_IF)
      {
         /* An #if pushes a copy of the current frame on the stack */
         cpd.pp_level++;
         pf_push(frm);
         frm->in_ifdef = CT_PP_IF;
         txt           = "if-push";
      }
      else if (pc->parent_type == CT_PP_ELSE)
      {
         pp_level--;

         /**
          * For #else of #elif, we want to keep the #if part and throw out the
          * else parts.
          * We check to see what the top type is to see if we just push or
          * pop and then push.
          * We need to use the copy right before the if.
          */
         if (frm->in_ifdef == CT_PP_IF)
         {
            /* we have [...] [base]-[if], so push an [else] */
            pf_push(frm);
            frm->in_ifdef = CT_PP_ELSE;
         }
         /* we have [...] [base] [if]-[else], copy [base] over [else] */
         pf_copy_2nd_tos(frm);
         frm->in_ifdef = CT_PP_ELSE;
         txt           = "else-push";
      }
      else if (pc->parent_type == CT_PP_ENDIF)
      {
         /**
          * we may have [...] [base] [if]-[else] or [...] [base]-[if].
          * Throw out the [else].
          */
         cpd.pp_level--;
         pp_level--;

         if (frm->in_ifdef == CT_PP_ELSE)
         {
            /**
             * We have: [...] [base] [if]-[else]
             * We want: [...]-[if]
             */
            pf_copy_tos(frm);     /* [...] [base] [if]-[if] */
            frm->in_ifdef = cpd.frames[cpd.frame_count - 2].in_ifdef;
            pf_trash_tos();       /* [...] [base]-[if] */
            pf_trash_tos();       /* [...]-[if] */

            txt = "endif-trash/pop";
         }
         else if (frm->in_ifdef == CT_PP_IF)
         {
            /**
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

   if (txt != NULL)
   {
      LOG_FMT(LPF, "%s: %d> %s: %s in_ifdef=%d/%d counts=%d/%d\n", __func__,
              pc->orig_line, get_token_name(pc->parent_type), txt,
              in_ifdef, frm->in_ifdef, b4_cnt, cpd.frame_count);
      pf_log_all(LPF);
      LOG_FMT(LPF, " <Out>");
      pf_log(LPF, frm);
   }

   pf_log_frms(LPFCHK, "END", frm);

   return(pp_level);
}
