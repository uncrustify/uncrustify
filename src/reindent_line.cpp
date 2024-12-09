/**
 * @file reindent.cpp
 * Does all the indenting stuff.
 *
 * @author  Guy Maurel October 2015- 2023
 * extract from indent.cpp
 * @license GPL v2+
 */
#include "reindent_line.h"

#include "chunk.h"
#include "log_rules.h"
#include "options_for_QT.h"
#include "space.h"
#include "uncrustify.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32

#ifdef IGNORE // WinBase.h
#undef IGNORE
#endif


constexpr static auto LCURRENT = LINDENT;

using namespace std;
using namespace uncrustify;


void reindent_line(Chunk *pc, size_t column)
{
   LOG_FUNC_ENTRY();
   char copy[1000];

   LOG_FMT(LINDLINE, "%s(%d): orig line is %zu, orig col is %zu, on '%s' [%s/%s] => %zu\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy),
           get_token_name(pc->GetType()), get_token_name(pc->GetParentType()),
           column);
   log_func_stack_inline(LINDLINE);

   if (column == pc->GetColumn())
   {
      return;
   }
   int    col_delta = column - pc->GetColumn();
   size_t min_col   = column;

   pc->SetColumn(column);

   do
   {
      if (QT_SIGNAL_SLOT_found)
      {
         // fix the bug #654
         // connect(&mapper, SIGNAL(mapped(QString &)), this, SLOT(onSomeEvent(QString &)));
         // look for end of SIGNAL/SLOT block
         if (!pc->TestFlags(PCF_IN_QT_MACRO))
         {
            LOG_FMT(LINDLINE, "FLAGS is NOT set: PCF_IN_QT_MACRO\n");
            restore_options_for_QT();
         }
      }
      else
      {
         // look for begin of SIGNAL/SLOT block
         if (pc->TestFlags(PCF_IN_QT_MACRO))
         {
            LOG_FMT(LINDLINE, "FLAGS is set: PCF_IN_QT_MACRO\n");
            save_set_options_for_QT(pc->GetLevel());
         }
      }
      Chunk *next = pc->GetNext();

      if (next->IsNullChunk())
      {
         break;
      }

      if (pc->GetNlCount() > 0)
      {
         min_col   = 0;
         col_delta = 0;
      }
      min_col += space_col_align(pc, next);
      pc       = next;

      const bool is_comment = pc->IsComment();
      log_rule_B("indent_relative_single_line_comments");
      const bool keep = (  is_comment
                        && pc->IsSingleLineComment()
                        && options::indent_relative_single_line_comments());

      if (  is_comment
         && pc->GetParentType() != CT_COMMENT_EMBED
         && !keep)
      {
         pc->SetColumn(max(pc->GetOrigCol(), min_col));
         LOG_FMT(LINDLINE, "%s(%d): set comment on line %zu to col %zu (orig %zu)\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetColumn(), pc->GetOrigCol());
      }
      else
      {
         pc->SetColumn(max(pc->GetColumn() + col_delta, min_col));

         LOG_FMT(LINDLINED, "%s(%d): set column of ", __func__, __LINE__);

         if (pc->Is(CT_NEWLINE))
         {
            LOG_FMT(LINDLINED, "<Newline>");
         }
         else
         {
            LOG_FMT(LINDLINED, "'%s'", pc->Text());
         }
         LOG_FMT(LINDLINED, " to %zu (orig %zu/%zu)\n", pc->GetColumn(), pc->GetOrigLine(), pc->GetOrigCol());
      }
   } while (  pc->IsNotNullChunk()
           && pc->GetNlCount() == 0);
} // reindent_line
