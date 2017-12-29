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


static void pf_log_frms(log_sev_t logsev, const char *txt, const ParseFrame &frm);


//! Logs the entire parse frame stack
static void pf_log_all(log_sev_t logsev);


//! Logs one parse frame
static void pf_log(log_sev_t logsev, const ParseFrame &frm)
{
   LOG_FMT(logsev, "[%s] BrLevel=%zu Level=%zu PseTos=%zu\n",
           get_token_name(frm.in_ifdef), frm.brace_level, frm.level, frm.size() - 1);

   LOG_FMT(logsev, " *");
   for (size_t idx = 1; idx < frm.size(); idx++)
   {
      LOG_FMT(logsev, " [%s-%u]", get_token_name(frm.at(idx).type),
              static_cast<unsigned int>(frm.at(idx).stage));
   }
   LOG_FMT(logsev, "\n");
}


static void pf_log_frms(log_sev_t logsev, const char *txt, const ParseFrame &frm)
{
   LOG_FMT(logsev, "%s Parse Frames(%zu):", txt, cpd.frame_count);

   for (size_t idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, " [%s-%zu]", get_token_name(cpd.frames[idx].in_ifdef),
              cpd.frames[idx].ref_no);
   }

   LOG_FMT(logsev, "-[%s-%zu]\n", get_token_name(frm.in_ifdef), frm.ref_no);
}


static void pf_log_all(log_sev_t logsev)
{
   LOG_FMT(logsev, "##=- Parse Frame : %zu entries\n", cpd.frame_count);

   for (size_t idx = 0; idx < cpd.frame_count; idx++)
   {
      LOG_FMT(logsev, "##  idx is %zu, ", idx);

      pf_log(logsev, cpd.frames[idx]);
   }
   LOG_FMT(logsev, "##=-\n");
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


void pf_push(ParseFrame &frm)
{
   static int ref_no = 1;

   cpd.frames[cpd.frame_count] = frm;
   cpd.frame_count++;
   controlFrameCount();
   frm.ref_no = ref_no++;
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_push_under(ParseFrame &frm)
{
   LOG_FMT(LPF, "%s(%d): before frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);

   if (cpd.frame_count >= 1)
   {
      ParseFrame &prev = cpd.frames[cpd.frame_count - 1];
      ParseFrame &top  = cpd.frames[cpd.frame_count];

      top  = prev;
      prev = frm;

      cpd.frame_count++;
      controlFrameCount();
   }

   LOG_FMT(LPF, "%s(%d): after frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_copy_tos(ParseFrame &pf)
{
   if (cpd.frame_count > 0)
   {
      pf = cpd.frames[cpd.frame_count - 1];
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frame_count);
}


void pf_copy_2nd_tos(ParseFrame &pf)
{
   if (cpd.frame_count > 1)
   {
      pf = cpd.frames[cpd.frame_count - 2];
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


void pf_pop(ParseFrame &pf)
{
   if (cpd.frame_count > 0)
   {
      pf_copy_tos(pf);
      pf_trash_tos();
   }
}


int pf_check(ParseFrame &frm, chunk_t *pc)
{
   if (pc->type != CT_PREPROC)
   {
      return(cpd.pp_level);
   }

   chunk_t *next = chunk_get_next(pc);
   if (next == nullptr)
   {
      return(cpd.pp_level);
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


   int             pp_level = cpd.pp_level;
   const c_token_t in_ifdef = frm.in_ifdef;
   const size_t    b4_cnt   = cpd.frame_count;

   const char      *txt = nullptr;
   if (pc->flags & PCF_IN_PREPROC)
   {
      LOG_FMT(LPF, " <In> ");
      pf_log(LPF, frm);

      if (pc->parent_type == CT_PP_IF)
      {
         // An #if pushes a copy of the current frame on the stack
         cpd.pp_level++;
         pf_push(frm);
         frm.in_ifdef = CT_PP_IF;
         txt          = "if-push";
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
         if (frm.in_ifdef == CT_PP_IF)
         {
            // we have [...] [base]-[if], so push an [else]
            pf_push(frm);
            frm.in_ifdef = CT_PP_ELSE;
         }
         // we have [...] [base] [if]-[else], copy [base] over [else]
         pf_copy_2nd_tos(frm);
         frm.in_ifdef = CT_PP_ELSE;
         txt          = "else-push";
      }
      else if (pc->parent_type == CT_PP_ENDIF)
      {
         /*
          * we may have [...] [base] [if]-[else] or [...] [base]-[if].
          * Throw out the [else].
          */
         cpd.pp_level--;
         pp_level--;

         if (frm.in_ifdef == CT_PP_ELSE)
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
            frm.in_ifdef = cpd.frames[cpd.frame_count - 2].in_ifdef;
            pf_trash_tos();       // [...] [base]-[if]
            pf_trash_tos();       // [...]-[if]

            txt = "endif-trash/pop";
         }
         else if (frm.in_ifdef == CT_PP_IF)
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
      LOG_FMT(LPF, "%s(%d): orig_line is %zu, type is %s: %s in_ifdef is %d/%d, counts is %zu, frame_count is %zu\n",
              __func__, __LINE__, pc->orig_line,
              get_token_name(pc->parent_type), txt, static_cast<int>(in_ifdef),
              static_cast<int>(frm.in_ifdef), b4_cnt, cpd.frame_count);
      pf_log_all(LPF);
      LOG_FMT(LPF, " <Out>");
      pf_log(LPF, frm);
   }

   pf_log_frms(LPFCHK, "END", frm);

   return(pp_level);
} // pf_check
