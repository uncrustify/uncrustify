/**
 * @file frame_list.cpp
 * mainly used to handle preprocessor stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "frame_list.h"

#include "chunk_list.h"
#include "error_types.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{

void fl_log_frms(log_sev_t logsev, const char *txt, const ParseFrame &frm, const std::vector<ParseFrame> &frames);


//! Logs the entire parse frame stack
void fl_log_all(log_sev_t logsev, const std::vector<ParseFrame> &frames);


/**
 * Copy the top element of the frame list into the ParseFrame.
 *
 * If the frame list is empty nothing happens.
 *
 * This is called on #else and #elif.
 */
void fl_copy_tos(ParseFrame &pf, const std::vector<ParseFrame> &frames);


/**
 * Copy the 2nd top element off the list into the ParseFrame.
 * This is called on #else and #elif.
 * The stack contains [...] [base] [if] at this point.
 * We want to copy [base].
 */
void fl_copy_2nd_tos(ParseFrame &pf, const std::vector<ParseFrame> &frames);


//! Deletes the top element from the list.
void fl_trash_tos(std::vector<ParseFrame> &frames);


//! Logs one parse frame
void fl_log(log_sev_t logsev, const ParseFrame &frm)
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


void fl_log_frms(log_sev_t                     logsev,
                 const char                    *txt,
                 const ParseFrame              &frm,
                 const std::vector<ParseFrame> &frames)
{
   LOG_FMT(logsev, "%s Parse Frames(%zu):", txt, frames.size());

   for (size_t idx = 0; idx < frames.size(); idx++)
   {
      LOG_FMT(logsev, " [%s-%zu]", get_token_name(frames.at(idx).in_ifdef),
              frames.at(idx).ref_no);
   }

   LOG_FMT(logsev, "-[%s-%zu]\n", get_token_name(frm.in_ifdef), frm.ref_no);
}


void fl_log_all(log_sev_t logsev, const std::vector<ParseFrame> &frames)
{
   LOG_FMT(logsev, "##=- Parse Frame : %zu entries\n", frames.size());

   for (size_t idx = 0; idx < frames.size(); idx++)
   {
      LOG_FMT(logsev, "##  idx is %zu, ", idx);

      fl_log(logsev, frames.at(idx));
   }

   LOG_FMT(logsev, "##=-\n");
}


void fl_copy_tos(ParseFrame &pf, const std::vector<ParseFrame> &frames)
{
   if (!frames.empty())
   {
      pf = *std::prev(std::end(frames));
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}


void fl_copy_2nd_tos(ParseFrame &pf, const std::vector<ParseFrame> &frames)
{
   if (frames.size() > 1)
   {
      pf = *std::prev(std::end(frames), 2);
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}


void fl_trash_tos(std::vector<ParseFrame> &frames)
{
   if (!frames.empty())
   {
      frames.pop_back();
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}

} // namespace


void fl_push(std::vector<ParseFrame> &frames, ParseFrame &frm)
{
   static int ref_no = 1;

   frames.push_back(frm);
   frm.ref_no = ref_no++;

   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}


void fl_pop(std::vector<ParseFrame> &frames, ParseFrame &pf)
{
   if (frames.empty())
   {
      return;
   }
   fl_copy_tos(pf, frames);
   fl_trash_tos(frames);
}


int fl_check(std::vector<ParseFrame> &frames, ParseFrame &frm, int &pp_level, chunk_t *pc)
{
   if (pc->type != CT_PREPROC)
   {
      return(pp_level);
   }
   chunk_t *next = chunk_get_next(pc);

   if (next == nullptr)
   {
      return(pp_level);
   }

   if (get_chunk_parent_type(pc) != next->type)
   {
      LOG_FMT(LNOTE, "%s(%d): Preproc parent not set correctly on orig_line %zu: got %s expected %s\n",
              __func__, __LINE__, pc->orig_line, get_token_name(get_chunk_parent_type(pc)),
              get_token_name(next->type));
      set_chunk_parent(pc, next->type);
   }
   LOG_FMT(LPFCHK, "%s(%d): %zu] %s\n",
           __func__, __LINE__, pc->orig_line, get_token_name(get_chunk_parent_type(pc)));
   fl_log_frms(LPFCHK, "TOP", frm, frames);


   int             out_pp_level = pp_level;
   const c_token_t in_ifdef     = frm.in_ifdef;
   const size_t    b4_cnt       = frames.size();

   const char      *txt = nullptr;

   if (pc->flags.test(PCF_IN_PREPROC))
   {
      LOG_FMT(LPF, " <In> ");
      fl_log(LPF, frm);

      if (get_chunk_parent_type(pc) == CT_PP_IF)
      {
         // An #if pushes a copy of the current frame on the stack
         pp_level++;
         fl_push(frames, frm);
         frm.in_ifdef = CT_PP_IF;
         txt          = "if-push";
      }
      else if (get_chunk_parent_type(pc) == CT_PP_ELSE)
      {
         if (out_pp_level == 0)
         {
            fprintf(stderr, "%s(%d): pp_level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         out_pp_level--;

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
            fl_push(frames, frm);
            frm.in_ifdef = CT_PP_ELSE;
         }
         // we have [...] [base] [if]-[else], copy [base] over [else]
         fl_copy_2nd_tos(frm, frames);
         frm.in_ifdef = CT_PP_ELSE;
         txt          = "else-push";
      }
      else if (get_chunk_parent_type(pc) == CT_PP_ENDIF)
      {
         /*
          * we may have [...] [base] [if]-[else] or [...] [base]-[if].
          * Throw out the [else].
          */
         if (pp_level == 0)
         {
            // cpd.pp_level is ZERO, cannot be decremented.
            fprintf(stderr, "%s(%d): #endif found, at line %zu, column %zu, without corresponding #if\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         pp_level--;

         if (out_pp_level == 0)
         {
            fprintf(stderr, "%s(%d): pp_level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         out_pp_level--;

         if (frm.in_ifdef == CT_PP_ELSE)
         {
            /*
             * We have: [...] [base] [if]-[else]
             * We want: [...]-[if]
             */
            fl_copy_tos(frm, frames);     // [...] [base] [if]-[if]

            if (frames.size() < 2)
            {
               fprintf(stderr, "Number of 'frame' is too small.\n");
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            frm.in_ifdef = frames[frames.size() - 2].in_ifdef;
            fl_trash_tos(frames);       // [...] [base]-[if]
            fl_trash_tos(frames);       // [...]-[if]

            txt = "endif-trash/pop";
         }
         else if (frm.in_ifdef == CT_PP_IF)
         {
            /*
             * We have: [...] [base] [if]
             * We want: [...] [base]
             */
            fl_pop(frames, frm);
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
              get_token_name(get_chunk_parent_type(pc)), txt, static_cast<int>(in_ifdef),
              static_cast<int>(frm.in_ifdef), b4_cnt, frames.size());
      fl_log_all(LPF, frames);
      LOG_FMT(LPF, " <Out>");
      fl_log(LPF, frm);
   }
   fl_log_frms(LPFCHK, "END", frm, frames);

   return(out_pp_level);
} // fl_check
