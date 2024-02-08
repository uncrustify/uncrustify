/**
 * @file assign.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/assign.h"

#include "align/stack.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALASS;


using namespace uncrustify;


Chunk *align_assign(Chunk *first, size_t span, size_t thresh, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (first->IsNullChunk())
   {
      // coveralls will complain here. There are no example for that.
      // see https://en.wikipedia.org/wiki/Robustness_principle
      return(Chunk::NullChunkPtr);
   }
   size_t my_level = first->GetLevel();

   char   copy[1000];

   LOG_FMT(LALASS, "%s(%d): [my_level is %zu]: start checking with '%s', on orig line %zu, span is %zu, thresh is %zu\n",
           __func__, __LINE__, my_level, first->ElidedText(copy), first->GetOrigLine(), span, thresh);

   // If we are aligning on a tabstop, we shouldn't right-align
   AlignStack as;    // regular assigns

   as.Start(span, thresh);
   log_rule_B("align_on_tabstop");
   as.m_right_align = !options::align_on_tabstop();

   AlignStack vdas;  // variable def assigns

   vdas.Start(span, thresh);
   vdas.m_right_align = as.m_right_align;

   std::deque<AlignStack> fcnDefault(1);

   fcnDefault.back().Start(span, thresh);
   fcnDefault.back().m_right_align = as.m_right_align;

   AlignStack fcnProto;

   fcnProto.Start(span, thresh);
   fcnProto.m_right_align = as.m_right_align;

   size_t var_def_cnt = 0;
   size_t equ_count   = 0;
   size_t nl_count    = 0;
   size_t fcn_idx     = 0;
   size_t tmp;
   Chunk  *pc      = first;
   Chunk  *vdas_pc = Chunk::NullChunkPtr;

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LALASS, "%s(%d): orig line is %zu, check pc->Text() is '%s', type is %s, m_parentType is %s\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->ElidedText(copy), get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));

      if (nl_count != 0)
      {
         if (vdas_pc->IsNotNullChunk())
         {
            LOG_FMT(LALASS, "%s(%d): vdas.Add on '%s' on orig line %zu, orig col is %zu\n",
                    __func__, __LINE__, vdas_pc->Text(), vdas_pc->GetOrigLine(), vdas_pc->GetOrigCol());
            vdas.Add(vdas_pc);
            vdas_pc = Chunk::NullChunkPtr;
         }

         if (p_nl_count != nullptr)
         {
            *p_nl_count += nl_count;
         }
         as.NewLines(nl_count);
         vdas.NewLines(nl_count);
         fcnProto.NewLines(nl_count);

         for (auto &fcn : fcnDefault)
         {
            fcn.NewLines(nl_count);
         }

         fcn_idx     = 0;
         nl_count    = 0;
         var_def_cnt = 0;
         equ_count   = 0;
      }

      // Don't check inside SPAREN, PAREN or SQUARE groups
      if (  pc->Is(CT_SPAREN_OPEN)
            // || pc->Is(CT_FPAREN_OPEN) Issue #1340
         || pc->Is(CT_SQUARE_OPEN)
         || pc->Is(CT_PAREN_OPEN))
      {
         LOG_FMT(LALASS, "%s(%d): Don't check inside SPAREN, PAREN or SQUARE groups, type is %s\n",
                 __func__, __LINE__, get_token_name(pc->GetType()));
         tmp = pc->GetOrigLine();
         pc  = pc->GetClosingParen();

         if (pc->IsNotNullChunk())
         {
            nl_count = pc->GetOrigLine() - tmp;
         }
         continue;
      }

      // Recurse if a brace set is found
      if (  (  pc->Is(CT_BRACE_OPEN)
            || pc->Is(CT_VBRACE_OPEN))
         && !(pc->GetParentType() == CT_BRACED_INIT_LIST))
      {
         size_t myspan;
         size_t mythresh;

         if (pc->GetParentType() == CT_ENUM)
         {
            log_rule_B("align_enum_equ_span");
            myspan = options::align_enum_equ_span();
            log_rule_B("align_enum_equ_thresh");
            mythresh = options::align_enum_equ_thresh();
         }
         else
         {
            log_rule_B("align_assign_span");
            myspan = options::align_assign_span();
            log_rule_B("align_assign_thresh");
            mythresh = options::align_assign_thresh();
         }
         pc = align_assign(pc->GetNext(), myspan, mythresh, &nl_count);
         continue;
      }

      // Done with this brace set?
      if (  (  pc->Is(CT_BRACE_CLOSE)
            || pc->Is(CT_VBRACE_CLOSE))
         && !(pc->GetParentType() == CT_BRACED_INIT_LIST))
      {
         pc = pc->GetNext();
         break;
      }

      if (pc->IsNewline())
      {
         nl_count = pc->GetNlCount();
      }
      else if (  pc->TestFlags(PCF_VAR_DEF)
              && !pc->TestFlags(PCF_IN_CONST_ARGS) // Issue #1717
              && !pc->TestFlags(PCF_IN_FCN_DEF)    // Issue #1717
              && !pc->TestFlags(PCF_IN_FCN_CALL))  // Issue #1717
      {
         // produces much more log output. Use it only debugging purpose
         //LOG_FMT(LALASS, "%s(%d): log_pcf_flags pc->GetFlags():\n   ", __func__, __LINE__);
         //log_pcf_flags(LALASS, pc->GetFlags());
         var_def_cnt++;
      }
      else if (  var_def_cnt > 1
              && !options::align_assign_on_multi_var_defs())
      {
         // we hit the second variable def and align was not requested - don't look for assigns, don't align
         LOG_FMT(LALASS, "%s(%d): multiple var defs found and alignment was not requested\n",
                 __func__, __LINE__);
         vdas_pc = Chunk::NullChunkPtr;
      }
      else if (  equ_count == 0                  // indent only if first '=' in line
              && !pc->TestFlags(PCF_IN_TEMPLATE) // and it is not inside a template #999
              && (  pc->Is(CT_ASSIGN)
                 || pc->Is(CT_ASSIGN_DEFAULT_ARG)
                 || pc->Is(CT_ASSIGN_FUNC_PROTO)))
      {
         if (pc->Is(CT_ASSIGN))               // Issue #2236
         {
            equ_count++;
         }
         LOG_FMT(LALASS, "%s(%d): align_assign_decl_func() is %d\n",
                 __func__, __LINE__, options::align_assign_decl_func());
         // produces much more log output. Use it only debugging purpose
         //LOG_FMT(LALASS, "%s(%d): log_pcf_flags pc->GetFlags(): ", __func__, __LINE__);
         //log_pcf_flags(LALASS, pc->GetFlags());

         log_rule_B("align_assign_decl_func");

         if (  options::align_assign_decl_func() == 0 // Align with other assignments (default)
            && (  pc->Is(CT_ASSIGN_DEFAULT_ARG)       // Foo( int bar = 777 );
               || pc->Is(CT_ASSIGN_FUNC_PROTO)))      // Foo( const Foo & ) = delete;
         {
            LOG_FMT(LALASS, "%s(%d): fcnDefault[%zu].Add on '%s' on orig line %zu, orig col is %zu\n",
                    __func__, __LINE__, fcn_idx, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

            if (++fcn_idx == fcnDefault.size())
            {
               fcnDefault.emplace_back();
               fcnDefault.back().Start(span, thresh);
               fcnDefault.back().m_right_align = as.m_right_align;
            }
            fcnDefault[fcn_idx].Add(pc);
         }
         else if (options::align_assign_decl_func() == 1)   // Align with each other
         {
            log_rule_B("align_assign_decl_func");

            if (pc->Is(CT_ASSIGN_DEFAULT_ARG))  // Foo( int bar = 777 );
            {
               LOG_FMT(LALASS, "%s(%d): default: fcnDefault[%zu].Add on '%s' on orig line %zu, orig col is %zu\n",
                       __func__, __LINE__, fcn_idx, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

               if (++fcn_idx == fcnDefault.size())
               {
                  fcnDefault.emplace_back();
                  fcnDefault.back().Start(span, thresh);
                  fcnDefault.back().m_right_align = as.m_right_align;
               }
               fcnDefault[fcn_idx].Add(pc);
            }
            else if (pc->Is(CT_ASSIGN_FUNC_PROTO))  // Foo( const Foo & ) = delete;
            {
               LOG_FMT(LALASS, "%s(%d): proto: fcnProto.Add on '%s' on orig line %zu, orig col is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
               fcnProto.Add(pc);
            }
            else if (pc->Is(CT_ASSIGN)) // Issue #2197
            {
               vdas_pc = pc;
            }
         }
         else if (  options::align_assign_decl_func() == 2 // Don't align
                 && (  pc->Is(CT_ASSIGN_DEFAULT_ARG)       // Foo( int bar = 777 );
                    || pc->Is(CT_ASSIGN_FUNC_PROTO)))      // Foo( const Foo & ) = delete;
         {
            log_rule_B("align_assign_decl_func");
            LOG_FMT(LALASS, "%s(%d): Don't align\n",               // Issue #2236
                    __func__, __LINE__);
         }
         else if (var_def_cnt != 0)
         {
            vdas_pc = pc;
         }
         else
         {
            if (pc->Is(CT_ASSIGN))
            {
               LOG_FMT(LALASS, "%s(%d): as.Add on '%s' on orig line %zu, orig col is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
               as.Add(pc);
            }
         }
      }
      pc = pc->GetNext();
   }

   if (vdas_pc->IsNotNullChunk())
   {
      LOG_FMT(LALASS, "%s(%d): vdas.Add on '%s' on orig line %zu, orig col is %zu\n",
              __func__, __LINE__, vdas_pc->Text(), vdas_pc->GetOrigLine(), vdas_pc->GetOrigCol());
      vdas.Add(vdas_pc);
      vdas_pc = Chunk::NullChunkPtr;
   }
   as.End();
   vdas.End();

   for (auto &fcn : fcnDefault)
   {
      fcn.End();
   }

   fcnProto.End();

   if (pc->IsNotNullChunk())
   {
      LOG_FMT(LALASS, "%s(%d): done on '%s' on orig line %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine());
   }
   else
   {
      LOG_FMT(LALASS, "%s(%d): done on NULL\n", __func__, __LINE__);
   }
   return(pc);
} // align_assign
