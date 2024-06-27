/**
 * @file pcf_flags.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "pcf_flags.h"

static const char *pcf_names[] =
{
   "IN_PREPROC",        // 0
   "IN_STRUCT",         // 1
   "IN_ENUM",           // 2
   "IN_FCN_DEF",        // 3
   "IN_FCN_CALL",       // 4
   "IN_SPAREN",         // 5
   "IN_TEMPLATE",       // 6
   "IN_TYPEDEF",        // 7
   "IN_CONST_ARGS",     // 8
   "IN_ARRAY_ASSIGN",   // 9
   "IN_CLASS",          // 10
   "IN_CLASS_BASE",     // 11
   "IN_NAMESPACE",      // 12
   "IN_FOR",            // 13
   "IN_OC_MSG",         // 14
   "IN_WHERE_SPEC",     // 15
   "IN_DECLTYPE",       // 16
   "FORCE_SPACE",       // 17
   "STMT_START",        // 18
   "EXPR_START",        // 19
   "DONT_INDENT",       // 20
   "ALIGN_START",       // 21
   "WAS_ALIGNED",       // 22
   "VAR_TYPE",          // 23
   "VAR_DEF",           // 24
   "VAR_1ST",           // 25
   "VAR_INLINE",        // 26
   "RIGHT_COMMENT",     // 27
   "OLD_FCN_PARAMS",    // 28
   "LVALUE",            // 29
   "ONE_LINER",         // 30
   "EMPTY_BODY",        // 31
   "ANCHOR",            // 32
   "PUNCTUATOR",        // 33
   "INSERTED",          // 34
   "LONG_BLOCK",        // 35
   "OC_BOXED",          // 36
   "KEEP_BRACE",        // 37
   "OC_RTYPE",          // 38
   "OC_ATYPE",          // 39
   "WF_ENDIF",          // 40
   "IN_QT_MACRO",       // 41
   "IN_FCN_CTOR",       // 42                    Issue #2152
   "IN_TRY_BLOCK",      // 43                    Issue #1734
   "INCOMPLETE",        // 44
   "IN_LAMBDA",         // 45
   "WF_IF",             // 46
   "NOT_POSSIBLE",      // 47
   "IN_CONDITIONAL",    // 48                    Issue #3558
   "OC_IN_BLOCK",       // 49
   "CONT_LINE",         // 50
};


std::string pcf_flags_str(PcfFlags flags)
{
   char buffer[64];

   // Generate hex representation first
   snprintf(buffer, 63, "[0x%llx:", (long long unsigned int)(flags));

   // Add human-readable names
   auto out   = std::string{ buffer };
   auto first = true;

   for (size_t i = 0; i < ARRAY_SIZE(pcf_names); ++i)
   {
      if (flags & static_cast<E_PcfFlag>(pcf_bit(i)))
      {
         if (first)
         {
            first = false;
         }
         else
         {
            out += ',';
         }
         out += pcf_names[i];
      }
   }

   out += ']';
   return(out);
}


void log_pcf_flags(log_sev_t sev, PcfFlags flags)
{
   if (!log_sev_on(sev))
   {
      return;
   }
   LOG_FMT(sev, "   chunk flags: %s\n", pcf_flags_str(flags).c_str());
}
