/**
 * @file parse_frame.c
 * @brief Does the parse frame stuff, which is used to handle #ifdef stuff
 *
 * $Id: parse_frame.c,v 1.8 2006/02/13 03:30:20 bengardner Exp $
 */

#include "cparse_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#define DEBUG_PF

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
   if (cpd.frame_count < ARRAY_SIZE(cpd.frames))
   {
      pf_copy(&cpd.frames[cpd.frame_count], pf);
      cpd.frame_count++;
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

   if ((cpd.frame_count < ARRAY_SIZE(cpd.frames)) &&
       (cpd.frame_count > 1))
   {
      npf1 = &cpd.frames[cpd.frame_count - 1];
      npf2 = &cpd.frames[cpd.frame_count];
      pf_copy(npf2, npf1);
      pf_copy(npf1, pf);
      cpd.frame_count++;
   }
   LOG_FMT(LPF, "%s: count = %d\n", __func__, cpd.frame_count);
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

void pf_check(struct parse_frame *frm, chunk_t *pc)
{
   int        in_ifdef = frm->in_ifdef;
   int        b4_cnt   = cpd.frame_count;
   const char *txt     = NULL;

   if ((pc->flags & PCF_IN_PREPROC) != 0)
   {
      if (pc->type == CT_PP_IF)
      {
         pf_push(frm);
         frm->in_ifdef = PP_IF;
         txt           = "push";
      }
      else if (pc->type == CT_PP_ELSE)
      {
         if (frm->in_ifdef == PP_IF)
         {
            /* need to switch */
            pf_push_under(frm);
            frm->in_ifdef = PP_ELSE;
            txt           = "push_under";
         }
         else if (frm->in_ifdef == PP_ELSE)
         {
            pf_copy_tos(frm);
            frm->in_ifdef = PP_ELSE;
            txt           = "copy";
         }
         else
         {
            txt = "???";
         }
      }
      else if (pc->type == CT_PP_ENDIF)
      {
         if (frm->in_ifdef == PP_ELSE)
         {
            pf_trash_tos();
            pf_pop(frm);
            txt = "trash/pop";
         }
         else if (frm->in_ifdef == PP_IF)
         {
            pf_pop(frm);
            txt = "pop";
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
              pc->orig_line, get_token_name(pc->type), txt,
              in_ifdef, frm->in_ifdef, b4_cnt, cpd.frame_count);
   }
}

