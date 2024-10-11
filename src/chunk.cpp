/**
 * @file chunk.cpp
 * Manages and navigates the list of chunks.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "chunk.h"

#include "ListManager.h"
#include "prototypes.h"
#include "space.h"

static ChunkListManager gChunkList;  // global chunk list


/*
 * Chunk class methods
 */

// Null Chunk
Chunk        Chunk::NullChunk(true);
Chunk *const Chunk::NullChunkPtr(&Chunk::NullChunk);


void Chunk::CopyFrom(const Chunk &o)
{
   m_type         = o.m_type;
   m_parentType   = o.m_parentType;
   m_origLine     = o.m_origLine;
   m_origCol      = o.m_origCol;
   m_origColEnd   = o.m_origColEnd;
   m_origPrevSp   = o.m_origPrevSp;
   m_column       = o.m_column;
   m_columnIndent = o.m_columnIndent;
   m_nlCount      = o.m_nlCount;
   m_nlColumn     = o.m_nlColumn;
   m_level        = o.m_level;
   m_braceLevel   = o.m_braceLevel;
   m_ppLevel      = o.m_ppLevel;
   m_afterTab     = o.m_afterTab;

   m_flags           = o.m_flags;
   m_alignmentData   = o.m_alignmentData;
   m_indentationData = o.m_indentationData;

   m_next   = Chunk::NullChunkPtr;
   m_prev   = Chunk::NullChunkPtr;
   m_parent = Chunk::NullChunkPtr;

   m_str          = o.m_str;
   m_trackingList = o.m_trackingList;
} // Chunk::CopyFrom


void Chunk::Reset()
{
   m_type         = CT_NONE;
   m_parentType   = CT_NONE;
   m_origLine     = 0;
   m_origCol      = 0;
   m_origColEnd   = 0;
   m_origPrevSp   = 0;
   m_column       = 0;
   m_columnIndent = 0;
   m_nlCount      = 0;
   m_nlColumn     = 0;
   m_level        = 0;
   m_braceLevel   = 0;
   m_ppLevel      = 999;            // use a big value to find some errors
   m_afterTab     = false;

   m_flags = PCF_NONE;
   memset(&m_alignmentData, 0, sizeof(m_alignmentData));
   m_alignmentData.next  = NullChunkPtr;
   m_alignmentData.start = NullChunkPtr;
   m_alignmentData.ref   = NullChunkPtr;
   memset(&m_indentationData, 0, sizeof(m_indentationData));

   m_next   = Chunk::NullChunkPtr;
   m_prev   = Chunk::NullChunkPtr;
   m_parent = Chunk::NullChunkPtr;

   // for debugging purpose only
   m_str.clear();
   m_trackingList = nullptr;
} // Chunk::Reset


const char *Chunk::ElidedText(char *for_the_copy) const
{
   const char *test_it       = Text();
   size_t     test_it_length = strlen(test_it);

   size_t     truncate_value = uncrustify::options::debug_truncate();

   if (truncate_value != 0)
   {
      if (test_it_length > truncate_value)
      {
         memset(for_the_copy, 0, 1000);

         if (test_it_length < truncate_value + 30)
         {
            strncpy(for_the_copy, test_it, truncate_value - 30);
            for_the_copy[truncate_value - 30] = 0;
         }
         else
         {
            strncpy(for_the_copy, test_it, truncate_value);
            for_the_copy[truncate_value] = 0;
         }
         char *message = strcat(for_the_copy, " ... <The string is truncated>");

         return(message);
      }
      else
      {
         return(test_it);
      }
   }
   return(test_it);
} // Chunk::ElidedText


Chunk *Chunk::GetNext(const E_Scope scope) const
{
   if (scope == E_Scope::ALL)
   {
      return(m_next);
   }
   Chunk *pc = m_next;

   if (TestFlags(PCF_IN_PREPROC))
   {
      // If in a preproc, return a null chunk if trying to leave
      if (!pc->TestFlags(PCF_IN_PREPROC))
      {
         return(NullChunkPtr);
      }
      return(pc);
   }

   // Not in a preproc, skip any preproc
   while (  pc->IsNotNullChunk()
         && pc->TestFlags(PCF_IN_PREPROC))
   {
      pc = pc->m_next;
   }
   return(pc);
} // Chunk::GetNext


Chunk *Chunk::GetPrev(const E_Scope scope) const
{
   if (scope == E_Scope::ALL)
   {
      return(m_prev);
   }
   Chunk *pc = m_prev;

   if (TestFlags(PCF_IN_PREPROC))
   {
      // If in a preproc, return a null chunk if trying to leave
      if (!pc->TestFlags(PCF_IN_PREPROC))
      {
         return(NullChunkPtr);
      }
      return(pc);
   }

   // Not in a preproc, skip any preproc
   while (  pc->IsNotNullChunk()
         && pc->TestFlags(PCF_IN_PREPROC))
   {
      pc = pc->m_prev;
   }
   return(pc);
} // Chunk::GetPrev


static void chunk_log(Chunk *pc, const char *text);


Chunk *Chunk::GetHead()
{
   return(gChunkList.GetHead());
} // Chunk::GetHead


Chunk *Chunk::GetTail()
{
   return(gChunkList.GetTail());
} // Chunk::GetTail


Chunk::T_SearchFnPtr Chunk::GetSearchFn(const E_Direction dir)
{
   return((dir == E_Direction::FORWARD) ? &Chunk::GetNext : &Chunk::GetPrev);
} // Chunk::GetSearchFn


Chunk *Chunk::Search(const T_CheckFnPtr checkFn, const E_Scope scope,
                     const E_Direction dir, const bool cond) const
{
   T_SearchFnPtr searchFnPtr = GetSearchFn(dir);
   Chunk         *pc         = const_cast<Chunk *>(this);

   //LOG_FMT(LSETTYP, "\n%s(%d): origLine is %zu, origCol is %zu\n",
   //        __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());

   do                                      // loop over the chunk list
   {
      pc = (pc->*searchFnPtr)(scope);      // in either direction while
      //LOG_FMT(LSETTYP, "%s(%d): origLine is %zu, origCol is %zu, type is %s\n",
      //        __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
   } while (  pc->IsNotNullChunk()         // the end of the list was not reached yet
           && ((pc->*checkFn)() != cond)); // and the demanded chunk was not found either

   return(pc);                             // the latest chunk is the searched one
} // Chunk::Search


bool Chunk::IsOnSameLine(const Chunk *end) const
{
   if (IsNullChunk())
   {
      return(false);
   }
   Chunk *tmp = GetNext();

   while (  tmp->IsNotNullChunk()
         && tmp != end)
   {
      if (tmp->Is(CT_NEWLINE))
      {
         return(false);
      }
      tmp = tmp->GetNext();
   }
   return(true);
} // Chunk::IsOnSameLine


Chunk *Chunk::SearchTypeLevel(const E_Token type, const E_Scope scope,
                              const E_Direction dir, const int level) const
{
   T_SearchFnPtr searchFnPtr = GetSearchFn(dir);
   Chunk         *pc         = const_cast<Chunk *>(this);

   do                                              // loop over the chunk list
   {
      pc = (pc->*searchFnPtr)(scope);              // in either direction while
   } while (  pc->IsNotNullChunk()                 // the end of the list was not reached yet
           && (!pc->IsTypeAndLevel(type, level))); // and the chunk was not found either

   return(pc);                                     // the latest chunk is the searched one
} // Chunk::SearchTypeLevel


Chunk *Chunk::SearchStringLevel(const char *str, const size_t len, int level,
                                const E_Scope scope, const E_Direction dir) const
{
   T_SearchFnPtr searchFnPtr = GetSearchFn(dir);
   Chunk         *pc         = const_cast<Chunk *>(this);

   do                                                        // loop over the chunk list
   {
      pc = (pc->*searchFnPtr)(scope);                        // in either direction while
   } while (  pc->IsNotNullChunk()                           // the end of the list was not reached yet
           && !pc->IsStringAndLevel(str, len, true, level)); // and the demanded chunk was not found either

   return(pc);                                               // the latest chunk is the searched one
} // Chunk::SearchStringLevel


Chunk *Chunk::SearchPpa(const T_CheckFnPtr checkFn, const bool cond) const
{
   if (!TestFlags(PCF_IN_PREPROC))
   {
      // if not in preprocessor, do a regular search
      return(Search(checkFn, E_Scope::ALL, E_Direction::FORWARD, cond));
   }
   Chunk *pc = GetNext();

   while (pc->IsNotNullChunk())
   {
      if (!pc->TestFlags(PCF_IN_PREPROC))
      {
         // Bail if we run off the end of the preprocessor directive, but return
         // the token because the caller may need to know where the search ended
         assert(pc->Is(CT_NEWLINE));
         return(pc);
      }

      if (pc->Is(CT_NL_CONT))
      {
         // Skip line continuation
         pc = pc->GetNext();
         continue;
      }

      if ((pc->*checkFn)() == cond)
      {
         // Requested token was found
         return(pc);
      }
      pc = pc->GetNext();
   }
   // Ran out of tokens
   return(Chunk::NullChunkPtr);
} // Chunk::SearchPpa


static void chunk_log_msg(Chunk *chunk, const log_sev_t log, const char *str)
{
   LOG_FMT(log, "%s orig line is %zu, orig col is %zu, ",
           str, chunk->GetOrigLine(), chunk->GetOrigCol());

   if (chunk->Is(CT_NEWLINE))
   {
      LOG_FMT(log, "<Newline>,\n");
   }
   else if (chunk->Is(CT_VBRACE_OPEN))
   {
      LOG_FMT(log, "<VBRACE_OPEN>,\n");
   }
   else if (chunk->Is(CT_VBRACE_CLOSE))
   {
      LOG_FMT(log, "<VBRACE_CLOSE>,\n");
   }
   else
   {
      LOG_FMT(log, "Text() is '%s', type is %s,\n", chunk->Text(), get_token_name(chunk->GetType()));
   }
} // chunk_log_msg


static void chunk_log(Chunk *pc, const char *text)
{
   if (  pc->IsNullChunk()
      || (  cpd.unc_stage != unc_stage_e::TOKENIZE
         && cpd.unc_stage != unc_stage_e::CLEANUP))
   {
      return;
   }
   const log_sev_t log   = LCHUNK;
   Chunk           *prev = pc->GetPrev();
   Chunk           *next = pc->GetNext();

   chunk_log_msg(pc, log, text);

   if (  prev->IsNotNullChunk()
      && next->IsNotNullChunk())
   {
      chunk_log_msg(prev, log, "   @ between");
      chunk_log_msg(next, log, "   and");
   }
   else if (next->IsNotNullChunk())
   {
      chunk_log_msg(next, log, "   @ before");
   }
   else if (prev->IsNotNullChunk())
   {
      chunk_log_msg(prev, log, "   @ after");
   }
   LOG_FMT(log, "   stage is %s",                             // Issue #3034
           get_unc_stage_name(cpd.unc_stage));
   log_func_stack_inline(log);
} // chunk_log


void Chunk::Delete(Chunk * &pc)
{
   gChunkList.Remove(pc);
   delete pc;
   pc = Chunk::NullChunkPtr;
} // Chunk::Delete


void Chunk::MoveAfter(Chunk *ref)
{
   LOG_FUNC_ENTRY();

   if (ref == this)
   {
      return;
   }
   gChunkList.Remove(this);
   gChunkList.AddAfter(this, ref);

   // Adjust the original column
   m_column     = ref->m_column + space_col_align(ref, this);
   m_origCol    = m_column;
   m_origColEnd = m_origCol + Len();
} // Chunk::MoveAfter


void Chunk::Swap(Chunk *other)
{
   gChunkList.Swap(this, other);
} // Chunk::Swap


bool Chunk::IsAddress() const
{
   if (  IsNotNullChunk()
      && (  Is(CT_BYREF)
         || (  Len() == 1
            && m_str[0] == '&'
            && IsNot(CT_OPERATOR_VAL))))
   {
      Chunk *prevc = GetPrev();

      if (  TestFlags(PCF_IN_TEMPLATE)
         && (  prevc->Is(CT_COMMA)
            || prevc->Is(CT_ANGLE_OPEN)))
      {
         return(false);
      }
      return(true);
   }
   return(false);
} // Chunk::IsAddress


Chunk *Chunk::GetFirstChunkOnLine() const
{
   Chunk *pc    = const_cast<Chunk *>(this);
   Chunk *first = pc;

   pc = pc->GetPrev();

   while (  pc->IsNotNullChunk()
         && !pc->IsNewline())
   {
      first = pc;
      pc    = pc->GetPrev();
   }
   return(first);
} // Chunk::GetFirstChunkOnLine


bool Chunk::IsLastChunkOnLine() const
{
   if (this == Chunk::GetTail())
   {
      return(true);
   }

   // if the next chunk is a newline then pc is the last chunk on its line
   if (GetNext()->Is(CT_NEWLINE))
   {
      return(true);
   }
   return(false);
} // Chunk::IsLastChunkOnLine


void Chunk::SwapLines(Chunk *other)
{
   // to swap lines we need to find the first chunk of the lines
   Chunk *pc1 = GetFirstChunkOnLine();
   Chunk *pc2 = other->GetFirstChunkOnLine();

   if (  pc1->IsNullChunk()
      || pc2->IsNullChunk()
      || pc1 == pc2)
   {
      return;
   }
   /*
    * Example start:
    * ? - start1 - a1 - b1 - nl1 - ? - ref2 - start2 - a2 - b2 - nl2 - ?
    *      ^- pc1                              ^- pc2
    */
   Chunk *ref2 = pc2->GetPrev();

   // Move the line started at pc2 before pc1
   while (  pc2->IsNotNullChunk()
         && !pc2->IsNewline())
   {
      Chunk *tmp = pc2->GetNext();
      gChunkList.Remove(pc2);
      gChunkList.AddBefore(pc2, pc1);
      pc2 = tmp;
   }
   /*
    * Should now be:
    * ? - start2 - a2 - b2 - start1 - a1 - b1 - nl1 - ? - ref2 - nl2 - ?
    *                         ^- pc1                              ^- pc2
    */

   // Now move the line started at pc1 after ref2
   while (  pc1->IsNotNullChunk()
         && !pc1->IsNewline())
   {
      Chunk *tmp = pc1->GetNext();
      gChunkList.Remove(pc1);

      if (ref2->IsNotNullChunk())
      {
         gChunkList.AddAfter(pc1, ref2);
      }
      else
      {
         gChunkList.AddHead(pc1);
      }
      ref2 = pc1;
      pc1  = tmp;
   }
   /*
    * Should now be:
    * ? - start2 - a2 - b2 - nl1 - ? - ref2 - start1 - a1 - b1 - nl2 - ?
    *                         ^- pc1                              ^- pc2
    */

   /*
    * pc1 and pc2 should be the newlines for their lines.
    * swap the chunks and the new line count so that the spacing remains the same.
    */
   if (  pc1->IsNotNullChunk()
      && pc2->IsNotNullChunk())
   {
      size_t nlCount = pc1->GetNlCount();

      pc1->SetNlCount(pc2->GetNlCount());
      pc2->SetNlCount(nlCount);

      pc1->Swap(pc2);
   }
} // Chunk::SwapLines


void Chunk::SetResetFlags(PcfFlags resetBits, PcfFlags setBits)
{
   if (IsNotNullChunk())
   {
      LOG_FUNC_ENTRY();
      const PcfFlags newFlags = (m_flags & ~resetBits) | setBits;

      if (m_flags != newFlags)
      {
         LOG_FMT(LSETFLG,
                 "%s(%d): %016llx^%016llx=%016llx\n"
                 "%s(%d): orig line is %zu, orig col is %zu, Text() is '%s', type is %s,",
                 __func__, __LINE__,
                 static_cast<PcfFlags::int_t>(m_flags),
                 static_cast<PcfFlags::int_t>(m_flags ^ newFlags),
                 static_cast<PcfFlags::int_t>(newFlags),
                 __func__, __LINE__, m_origLine, m_origCol, Text(), get_token_name(m_type));
         LOG_FMT(LSETFLG, "  parent type is %s,\n",
                 get_token_name(m_parentType));
         log_func_stack_inline(LSETFLG);

         LOG_FMT(LSETFLG, "  before: ");
         log_pcf_flags(LSETFLG, m_flags);
         LOG_FMT(LSETFLG, "  after:  ");
         log_pcf_flags(LSETFLG, newFlags);
         m_flags = newFlags;
      }
   }
} // Chunk::SetResetFlags


void Chunk::SetType(const E_Token token)
{
   LOG_FUNC_ENTRY();

   if (  IsNullChunk()
      || m_type == token)
   {
      return;
   }
   LOG_FMT(LSETTYP, "%s(%d): m_origLine is %zu, m_origCol is %zu, Text() is ",
           __func__, __LINE__, m_origLine, m_origCol);

   if (token == CT_NEWLINE)
   {
      LOG_FMT(LSETTYP, "<Newline>\n");
   }
   else if (token == CT_WHITESPACE)
   {
      LOG_FMT(LSETTYP, "<white_space>\n");
   }
   else
   {
      LOG_FMT(LSETTYP, "'%s'\n", Text());
   }
   LOG_FMT(LSETTYP, "   m_type is %s, m_parentType is %s => token is %s\n",
           get_token_name(m_type), get_token_name(m_parentType), get_token_name(token));
   m_type = token;
} // Chunk::SetType


void Chunk::SetParentType(const E_Token token)
{
   LOG_FUNC_ENTRY();

   if (  IsNullChunk()
      || m_parentType == token)
   {
      return;
   }
   LOG_FMT(LSETPAR, "%s(%d): orig line is %zu, orig col is %zu, Text() is ",
           __func__, __LINE__, m_origLine, m_origCol);

   if (token == CT_NEWLINE)
   {
      LOG_FMT(LSETPAR, "<Newline>\n");
   }
   else
   {
      LOG_FMT(LSETPAR, "'%s'\n", Text());
   }
   LOG_FMT(LSETPAR, "   type is %s, parent type is %s => new parent type is %s\n",
           get_token_name(m_type), get_token_name(m_parentType), get_token_name(token));
   m_parentType = token;
} // Chunk::SetParentType


Chunk *Chunk::CopyAndAdd(Chunk *pos, const E_Direction dir) const
{
#ifdef DEBUG
   // test if this chunk is properly set
   if (m_ppLevel == 999)
   {
      fprintf(stderr, "%s(%d): pp level is not set\n", __func__, __LINE__);
      log_func_stack_inline(LSETFLG);
      log_flush(true);
      exit(EX_SOFTWARE);
   }

   if (m_origLine == 0)
   {
      fprintf(stderr, "%s(%d): no line number\n", __func__, __LINE__);
      log_func_stack_inline(LSETFLG);
      log_flush(true);
      exit(EX_SOFTWARE);
   }

   if (m_origCol == 0)
   {
      fprintf(stderr, "%s(%d): no column number\n", __func__, __LINE__);
      log_func_stack_inline(LSETFLG);
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#endif /* DEBUG */

   Chunk *pc = new Chunk(*this);

   if (pos->IsNotNullChunk())
   {
      (dir == E_Direction::FORWARD) ? gChunkList.AddAfter(pc, pos) : gChunkList.AddBefore(pc, pos);
   }
   else
   {
      (dir == E_Direction::FORWARD) ? gChunkList.AddHead(pc) : gChunkList.AddTail(pc);
   }
   chunk_log(pc, "CopyAndAdd(A):");
   return(pc);
} // Chunk::CopyAndAdd


Chunk *Chunk::GetNextNbsb() const
{
   Chunk *pc = const_cast<Chunk *>(this);

   while (  pc->Is(CT_TSQUARE)
         || pc->Is(CT_SQUARE_OPEN))
   {
      if (pc->Is(CT_SQUARE_OPEN))
      {
         pc = pc->GetClosingParen();
      }
      pc = pc->GetNextNcNnl();
   }
   return(pc);
} // Chunk::GetNextNbsb


Chunk *Chunk::GetPrevNbsb() const
{
   Chunk *pc = const_cast<Chunk *>(this);

   while (  pc->Is(CT_TSQUARE)
         || pc->Is(CT_SQUARE_CLOSE))
   {
      if (pc->Is(CT_SQUARE_CLOSE))
      {
         pc = pc->GetOpeningParen();
      }
      pc = pc->GetPrevNcNnl();
   }
   return(pc);
} // Chunk::GetPrevNbsb


Chunk *Chunk::GetPpStart() const
{
   if (!IsPreproc())
   {
      return(Chunk::NullChunkPtr);
   }
   Chunk *pc = const_cast<Chunk *>(this);
   Chunk *prev;

   do
   {
      prev = pc;
      pc   = pc->GetPrev(E_Scope::PREPROC);
   } while (pc->IsPreproc());

   return(prev);
} // Chunk::GetPpStart


Chunk *Chunk::SkipDcMember() const
{
   LOG_FUNC_ENTRY();

   Chunk *pc  = const_cast<Chunk *>(this);
   Chunk *nxt = pc->Is(CT_DC_MEMBER) ? pc : pc->GetNextNcNnl(E_Scope::ALL);

   while (nxt->Is(CT_DC_MEMBER))
   {
      pc = nxt->GetNextNcNnl(E_Scope::ALL);

      if (pc->IsNullChunk())
      {
         return(Chunk::NullChunkPtr);
      }
      nxt = pc->GetNextNcNnl(E_Scope::ALL);
   }
   return(pc);
} // Chunk::SkipDcMember


int Chunk::ComparePosition(const Chunk *other) const
{
   if (m_origLine < other->m_origLine)
   {
      return(-1);
   }
   else if (m_origLine == other->m_origLine)
   {
      if (m_origCol < other->m_origCol)
      {
         return(-1);
      }
      else if (m_origCol == other->m_origCol)
      {
         return(0);
      }
   }
   return(1);
} // Chunk::ComparePosition


bool Chunk::IsOCForinOpenParen() const
{
   if (  language_is_set(lang_flag_e::LANG_OC)
      && Is(CT_SPAREN_OPEN)
      && GetPrevNcNnl()->Is(CT_FOR))
   {
      Chunk *nxt = const_cast<Chunk *>(this);

      while (  nxt->IsNotNullChunk()
            && nxt->IsNot(CT_SPAREN_CLOSE)
            && nxt->IsNot(CT_IN))
      {
         nxt = nxt->GetNextNcNnl();
      }

      if (nxt->Is(CT_IN))
      {
         return(true);
      }
   }
   return(false);
} // Chunk::IsOCForinOpenParen


bool Chunk::IsStringAndLevel(const char *str, const size_t len,
                             bool caseSensitive, const int level) const
{
   return(  (  level < 0
            || m_level == static_cast<size_t>(level))
         && Len() == len                                    // the length is as expected
         && (  (  caseSensitive
               && memcmp(Text(), str, len) == 0)
            || (  !caseSensitive
               && strncasecmp(Text(), str, len) == 0)));   // the strings are equal
} // Chunk::IsStringAndLevel


Chunk *Chunk::GetClosingParen(E_Scope scope) const
{
   if (  Is(CT_PAREN_OPEN)
      || Is(CT_SPAREN_OPEN)
      || Is(CT_FPAREN_OPEN)
      || Is(CT_TPAREN_OPEN)
      || Is(CT_BRACE_OPEN)
      || Is(CT_VBRACE_OPEN)
      || Is(CT_ANGLE_OPEN)
      || Is(CT_SQUARE_OPEN))
   {
      return(GetNextType((E_Token)(m_type + 1), m_level, scope));
   }
   return(const_cast<Chunk *>(this));
} // Chunk::GetClosingParen


Chunk *Chunk::GetOpeningParen(E_Scope scope) const
{
   if (  Is(CT_PAREN_CLOSE)
      || Is(CT_SPAREN_CLOSE)
      || Is(CT_FPAREN_CLOSE)
      || Is(CT_TPAREN_CLOSE)
      || Is(CT_BRACE_CLOSE)
      || Is(CT_VBRACE_CLOSE)
      || Is(CT_ANGLE_CLOSE)
      || Is(CT_SQUARE_CLOSE))
   {
      return(GetPrevType((E_Token)(m_type - 1), m_level, scope));
   }
   return(const_cast<Chunk *>(this));
} // Chunk::GetOpeningParen


bool Chunk::IsCppInheritanceAccessSpecifier() const
{
   return(  language_is_set(lang_flag_e::LANG_CPP)
         && (  Is(CT_ACCESS)
            || Is(CT_QUALIFIER))
         && (  IsString("private")
            || IsString("protected")
            || IsString("public")));
} // Chunk::IsCppInheritanceAccessSpecifier


bool Chunk::IsColon() const
{
   return(  Is(CT_ACCESS_COLON)
         || Is(CT_ASM_COLON)
         || Is(CT_BIT_COLON)
         || Is(CT_CASE_COLON)
         || Is(CT_CLASS_COLON)
         || Is(CT_COLON)
         || Is(CT_COND_COLON)
         || Is(CT_CONSTR_COLON)
         || Is(CT_CS_SQ_COLON)
         || Is(CT_D_ARRAY_COLON)
         || Is(CT_ENUM_COLON)
         || Is(CT_FOR_COLON)
         || Is(CT_LABEL_COLON)
         || Is(CT_OC_COLON)
         || Is(CT_OC_DICT_COLON)
         || Is(CT_TAG_COLON)
         || Is(CT_WHERE_COLON));
} // Chunk::IsColon


bool Chunk::IsDoxygenComment() const
{
   if (!IsComment())
   {
      return(false);
   }

   if (Len() < 3)
   {
      return(false);
   }
   // check the third character
   const char *sComment = Text();
   return(  (sComment[2] == '/')
         || (sComment[2] == '!')
         || (sComment[2] == '@'));
} // Chunk::IsDoxygenComment


bool Chunk::IsTypeDefinition() const
{
   return(  Is(CT_TYPE)
         || Is(CT_PTR_TYPE)
         || Is(CT_BYREF)
         || Is(CT_DC_MEMBER)
         || Is(CT_QUALIFIER)
         || Is(CT_STRUCT)
         || Is(CT_ENUM)
         || Is(CT_UNION));
} // Chunk::IsTypeDefinition


bool Chunk::IsNewlineBetween(const Chunk *other) const
{
   Chunk *pc = const_cast<Chunk *>(this);

   while (pc != other)
   {
      if (pc->IsNewline())
      {
         return(true);
      }
      pc = pc->GetNext();
   }
   return(false);
} // Chunk::IsNewlineBetween


void shift_the_rest_of_the_line(Chunk *first)
{
   // shift all the tokens in this line to the right  Issue #3236
   for (Chunk *temp = first; ; temp = temp->GetNext())
   {
      temp->SetColumn(temp->GetColumn() + 1);                         // Issue #3236
      temp->SetOrigCol(temp->GetOrigCol() + 1);                       // Issue #3236
      temp->SetOrigColEnd(temp->GetOrigColEnd() + 1);                 // Issue #3236

      if (temp->Is(CT_NEWLINE))
      {
         break;
      }
   }
} // shift_the_rest_of_the_line
