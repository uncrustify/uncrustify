/**
 * @file frame_list.cpp
 * Functions for the cpd.frames var, mainly used to handle preprocessor stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "uncrustify.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "frame_list.h"


static void fl_log_frms(log_sev_t logsev, const char *txt, const ParseFrame &frm);


//! Logs the entire parse frame stack
static void fl_log_all(log_sev_t logsev);


/**
 * Copy the top element of the frame list into the ParseFrame.
 *
 * If the frame list is empty nothing happens.
 *
 * This is called on #else and #elif.
 */
static void fl_copy_tos(ParseFrame &pf);


/**
 * Copy the 2nd top element off the list into the ParseFrame.
 * This is called on #else and #elif.
 * The stack contains [...] [base] [if] at this point.
 * We want to copy [base].
 */
static void fl_copy_2nd_tos(ParseFrame &pf);


//! Deletes the top element from the list.
static void fl_trash_tos(void);


//! Logs one parse frame
static void fl_log(log_sev_t logsev, const ParseFrame &frm)
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


static void fl_log_frms(log_sev_t logsev, const char *txt, const ParseFrame &frm)
{
   LOG_FMT(logsev, "%s Parse Frames(%zu):", txt, cpd.frames.size());

   for (size_t idx = 0; idx < cpd.frames.size(); idx++)
   {
      LOG_FMT(logsev, " [%s-%zu]", get_token_name(cpd.frames.at(idx).in_ifdef),
              cpd.frames.at(idx).ref_no);
   }

   LOG_FMT(logsev, "-[%s-%zu]\n", get_token_name(frm.in_ifdef), frm.ref_no);
}


static void fl_log_all(log_sev_t logsev)
{
   LOG_FMT(logsev, "##=- Parse Frame : %zu entries\n", cpd.frames.size());

   for (size_t idx = 0; idx < cpd.frames.size(); idx++)
   {
      LOG_FMT(logsev, "##  idx is %zu, ", idx);

      fl_log(logsev, cpd.frames.at(idx));
   }
   LOG_FMT(logsev, "##=-\n");
}


static void fl_copy_tos(ParseFrame &pf)
{
   if (!cpd.frames.empty())
   {
      pf = *std::prev(std::end(cpd.frames));
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frames.size());
}


static void fl_copy_2nd_tos(ParseFrame &pf)
{
   if (cpd.frames.size() > 1)
   {
      pf = *std::prev(std::end(cpd.frames), 2);
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frames.size());
}


static void fl_trash_tos(void)
{
   if (!cpd.frames.empty())
   {
      cpd.frames.pop_back();
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frames.size());
}


void fl_push(ParseFrame &frm)
{
   static int ref_no = 1;

   cpd.frames.push_back(frm);
   frm.ref_no = ref_no++;

   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, cpd.frames.size());
}


void fl_pop(ParseFrame &pf)
{
   if (!cpd.frames.empty())
   {
      fl_copy_tos(pf);
      fl_trash_tos();
   }
}


int fl_check(ParseFrame &frm, chunk_t *pc)
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
   fl_log_frms(LPFCHK, "TOP", frm);


   int             pp_level = cpd.pp_level;
   const c_token_t in_ifdef = frm.in_ifdef;
   const size_t    b4_cnt   = cpd.frames.size();

   const char      *txt = nullptr;
   if (pc->flags & PCF_IN_PREPROC)
   {
      LOG_FMT(LPF, " <In> ");
      fl_log(LPF, frm);

      if (pc->parent_type == CT_PP_IF)
      {
         // An #if pushes a copy of the current frame on the stack
         cpd.pp_level++;
         fl_push(frm);
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
            fl_push(frm);
            frm.in_ifdef = CT_PP_ELSE;
         }
         // we have [...] [base] [if]-[else], copy [base] over [else]
         fl_copy_2nd_tos(frm);
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
            fl_copy_tos(frm);     // [...] [base] [if]-[if]

            if (cpd.frames.size() < 2)
            {
               fprintf(stderr, "Number of 'frame' is too small.\n");
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            frm.in_ifdef = cpd.frames[cpd.frames.size() - 2].in_ifdef;
            fl_trash_tos();       // [...] [base]-[if]
            fl_trash_tos();       // [...]-[if]

            txt = "endif-trash/pop";
         }
         else if (frm.in_ifdef == CT_PP_IF)
         {
            /*
             * We have: [...] [base] [if]
             * We want: [...] [base]
             */
            fl_pop(frm);
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
              static_cast<int>(frm.in_ifdef), b4_cnt, cpd.frames.size());
      fl_log_all(LPF);
      LOG_FMT(LPF, " <Out>");
      fl_log(LPF, frm);
   }

   fl_log_frms(LPFCHK, "END", frm);

   return(pp_level);
} // pf_check
