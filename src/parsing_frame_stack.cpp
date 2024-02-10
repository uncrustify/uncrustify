/**
 * @file parsing_frame_stack.cpp
 * mainly used to handle preprocessor stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "parsing_frame_stack.h"

#include "prototypes.h"


using namespace uncrustify;


namespace
{

typedef std::vector<ParsingFrame> ParsingFrameOrigStack;

void fl_log_frms(log_sev_t logsev, const char *txt, const ParsingFrame &frm, const ParsingFrameOrigStack &frames);


//! Logs the entire parse frame stack
void fl_log_all(log_sev_t logsev, const ParsingFrameOrigStack &frames);


/**
 * Copy the top element of the frame list into the ParsingFrame.
 *
 * If the frame list is empty nothing happens.
 *
 * This is called on #else and #elif.
 */
void fl_copy_tos(ParsingFrame &pf, const ParsingFrameOrigStack &frames);


/**
 * Copy the 2nd top element off the list into the ParsingFrame.
 * This is called on #else and #elif.
 * The stack contains [...] [base] [if] at this point.
 * We want to copy [base].
 */
void fl_copy_2nd_tos(ParsingFrame &pf, const ParsingFrameOrigStack &frames);


//! Deletes the top element from the list.
void fl_trash_tos(ParsingFrameOrigStack &frames);


//! Logs one parse frame
void fl_log(log_sev_t logsev, const ParsingFrame &frm)
{
   LOG_FMT(logsev, "[%s] Brace level=%zu Paren level=%zu PseTos=%zu\n",
           get_token_name(frm.GetIfdefType()), frm.GetBraceLevel(), frm.GetParenLevel(), frm.size() - 1);

   LOG_FMT(logsev, " *");

   for (size_t idx = 1; idx < frm.size(); idx++)
   {
      LOG_FMT(logsev, " [%s-%s]", get_token_name(frm.at(idx).GetOpenToken()),
              get_brace_stage_name(frm.at(idx).GetStage()));
   }

   LOG_FMT(logsev, "\n");
}


void fl_log_frms(log_sev_t                   logsev,
                 const char                  *txt,
                 const ParsingFrame          &frm,
                 const ParsingFrameOrigStack &frames)
{
   LOG_FMT(logsev, "%s Parse Frames(%zu):", txt, frames.size());

   for (const auto &frame : frames)
   {
      LOG_FMT(logsev, " [%s-%zu]", get_token_name(frame.GetIfdefType()),
              frame.GetRefNumber());
   }

   LOG_FMT(logsev, "-[%s-%zu]\n", get_token_name(frm.GetIfdefType()), frm.GetRefNumber());
}


void fl_log_all(log_sev_t logsev, const ParsingFrameOrigStack &frames)
{
   LOG_FMT(logsev, "##=- Parse Frame : %zu entries\n", frames.size());

   for (size_t idx = 0; idx < frames.size(); idx++)
   {
      LOG_FMT(logsev, "##  idx is %zu, ", idx);

      fl_log(logsev, frames.at(idx));
   }

   LOG_FMT(logsev, "##=-\n");
}


void fl_copy_tos(ParsingFrame &pf, const ParsingFrameOrigStack &frames)
{
   if (!frames.empty())
   {
      pf = *std::prev(std::end(frames));
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}


void fl_copy_2nd_tos(ParsingFrame &pf, const ParsingFrameOrigStack &frames)
{
   if (frames.size() > 1)
   {
      pf = *std::prev(std::end(frames), 2);
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}


void fl_trash_tos(ParsingFrameOrigStack &frames)
{
   if (!frames.empty())
   {
      frames.pop_back();
   }
   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, frames.size());
}

} // namespace


ParsingFrameStack::ParsingFrameStack()
   : m_frames()
{
}


void ParsingFrameStack::push(ParsingFrame &frm)
{
   static int seq_ref_no = 1;

   m_frames.push_back(frm);
   frm.SetRefNumber(seq_ref_no++);

   LOG_FMT(LPF, "%s(%d): frame_count is %zu\n", __func__, __LINE__, m_frames.size());
}


void ParsingFrameStack::pop(ParsingFrame &pf)
{
   if (m_frames.empty())
   {
      return;
   }
   fl_copy_tos(pf, m_frames);
   fl_trash_tos(m_frames);
}


int ParsingFrameStack::check(ParsingFrame &frm, int &pp_level, Chunk *pc)
{
   if (pc->IsNot(CT_PREPROC))
   {
      return(pp_level);
   }
   Chunk *next = pc->GetNext();

   if (next->IsNullChunk())
   {
      return(pp_level);
   }

   if (pc->GetParentType() != next->GetType())
   {
      LOG_FMT(LNOTE, "%s(%d): Preproc parent not set correctly on orig line %zu: got %s expected %s\n",
              __func__, __LINE__, pc->GetOrigLine(), get_token_name(pc->GetParentType()),
              get_token_name(next->GetType()));
      pc->SetParentType(next->GetType());
   }
   LOG_FMT(LPFCHK, "%s(%d): orig line is %zu, %s\n",
           __func__, __LINE__, pc->GetOrigLine(), get_token_name(pc->GetParentType()));
   fl_log_frms(LPFCHK, "TOP", frm, m_frames);


   int           out_pp_level = pp_level;
   const E_Token in_ifdef     = frm.GetIfdefType();
   const size_t  b4_cnt       = m_frames.size();

   const char    *txt = nullptr;

   if (pc->TestFlags(PCF_IN_PREPROC))
   {
      LOG_FMT(LPF, " <In> ");
      fl_log(LPF, frm);

      if (pc->GetParentType() == CT_PP_IF)
      {
         // An #if pushes a copy of the current frame on the stack
         pp_level++;
         push(frm);
         frm.SetIfdefType(CT_PP_IF);
         txt = "if-push";
      }
      else if (pc->GetParentType() == CT_PP_ELSE)
      {
         if (out_pp_level == 0)
         {
            fprintf(stderr, "%s(%d): pp level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
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
         bool if_block = false;

         if (frm.GetIfdefType() == CT_PP_IF)
         {
            // we have [...] [base]-[if], so push an [else]
            push(frm);
            frm.SetIfdefType(CT_PP_ELSE);
            if_block = true;
         }
         size_t brace_level = frm.GetBraceLevel();
         // we have [...] [base] [if]-[else], copy [base] over [else]
         fl_copy_2nd_tos(frm, m_frames);
         frm.SetIfdefType(CT_PP_ELSE);

         if (if_block)
         {
            // check if #if block was unbalanced
            size_t base_brace_level = m_frames[m_frames.size() - 2].GetBraceLevel();

            if (brace_level != base_brace_level)
            {
               if (options::pp_unbalanced_if_action() > 0)
               {
                  LOG_FMT(LWARN, "%s(%d): orig line is %zu, unbalanced #if block braces (1), in-level is %zu, out-level is %zu\n",
                          __func__, __LINE__, pc->GetOrigLine(), base_brace_level, brace_level);
               }

               if (options::pp_unbalanced_if_action() == 2)
               {
                  exit(EX_SOFTWARE);
               }
            }
         }
         else
         {
            // check if previous #else block has a different indentation than the corresponding #if block
            size_t if_brace_level = m_frames[m_frames.size() - 1].GetBraceLevel();

            if (brace_level != if_brace_level)
            {
               if (options::pp_unbalanced_if_action() > 0)
               {
                  LOG_FMT(LWARN, "%s(%d): orig line is %zu, unbalanced #if-#else block braces (1), #else out-level is %zu, #if out-level is %zu\n",
                          __func__, __LINE__, pc->GetOrigLine(), brace_level, if_brace_level);
               }

               if (options::pp_unbalanced_if_action() == 2)
               {
                  exit(EX_SOFTWARE);
               }
            }
         }
         txt = "else-push";
      }
      else if (pc->GetParentType() == CT_PP_ENDIF)
      {
         /*
          * we may have [...] [base] [if]-[else] or [...] [base]-[if].
          * Throw out the [else].
          */
         if (pp_level == 0)
         {
            // cpd.pp_level is ZERO, cannot be decremented.
            fprintf(stderr, "%s(%d): #endif found, at line %zu, column %zu, without corresponding #if\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         pp_level--;

         if (out_pp_level == 0)
         {
            fprintf(stderr, "%s(%d): pp level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         out_pp_level--;

         if (frm.GetIfdefType() == CT_PP_ELSE)
         {
            size_t brace_level = frm.GetBraceLevel(); // brace level or current #else block
            /*
             * We have: [...] [base] [if]-[else]
             * We want: [...]-[if]
             */
            fl_copy_tos(frm, m_frames);               // [...] [base] [if]-[if]

            if (brace_level != frm.GetBraceLevel())
            {
               if (options::pp_unbalanced_if_action() > 0)
               {
                  LOG_FMT(LWARN, "%s(%d): orig line is %zu, unbalanced #if-#else block braces (2), #else out-level is %zu, #if out-level is %zu\n",
                          __func__, __LINE__, pc->GetOrigLine(), brace_level, frm.GetBraceLevel());
               }

               if (options::pp_unbalanced_if_action() == 2)
               {
                  exit(EX_SOFTWARE);
               }
            }

            if (m_frames.size() < 2)
            {
               fprintf(stderr, "Number of 'frame' is too small.\n");
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            frm.SetIfdefType(m_frames[m_frames.size() - 2].GetIfdefType());
            fl_trash_tos(m_frames);       // [...] [base]-[if]
            fl_trash_tos(m_frames);       // [...]-[if]

            txt = "endif-trash/pop";
         }
         else if (frm.GetIfdefType() == CT_PP_IF)
         {
            /*
             * We have: [...] [base] [if]
             * We want: [...] [base]
             */
            // check if #if block was unbalanced
            size_t brace_level = frm.GetBraceLevel();
            pop(frm);

            if (brace_level != frm.GetBraceLevel())
            {
               if (options::pp_unbalanced_if_action() > 0)
               {
                  LOG_FMT(LWARN, "%s(%d): orig line is %zu, unbalanced #if block braces (2), in-level is %zu, out-level is %zu\n",
                          __func__, __LINE__, pc->GetOrigLine(), frm.GetBraceLevel(), brace_level);
               }

               if (options::pp_unbalanced_if_action() == 2)
               {
                  exit(EX_SOFTWARE);
               }
            }
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
      LOG_FMT(LPF, "%s(%d): orig line is %zu, type is %s: %s ifdef token is %s/%s, counts is %zu, frame_count is %zu\n",
              __func__, __LINE__, pc->GetOrigLine(),
              get_token_name(pc->GetParentType()), txt, get_token_name(in_ifdef),
              get_token_name(frm.GetIfdefType()), b4_cnt, m_frames.size());
      fl_log_all(LPF, m_frames);
      LOG_FMT(LPF, " <Out>");
      fl_log(LPF, frm);
   }
   fl_log_frms(LPFCHK, "END", frm, m_frames);

   return(out_pp_level);
} // check
