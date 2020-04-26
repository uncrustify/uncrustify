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
   "FORCE_SPACE",       // 16
   "STMT_START",        // 17
   "EXPR_START",        // 18
   "DONT_INDENT",       // 19
   "ALIGN_START",       // 20
   "WAS_ALIGNED",       // 21
   "VAR_TYPE",          // 22
   "VAR_DEF",           // 23
   "VAR_1ST",           // 24
   "VAR_INLINE",        // 25
   "RIGHT_COMMENT",     // 26
   "OLD_FCN_PARAMS",    // 27
   "LVALUE",            // 28
   "ONE_LINER",         // 29
   "EMPTY_BODY",        // 30
   "ANCHOR",            // 31
   "PUNCTUATOR",        // 32
   "INSERTED",          // 33
   "LONG_BLOCK",        // 34
   "OC_BOXED",          // 35
   "KEEP_BRACE",        // 36
   "OC_RTYPE",          // 37
   "OC_ATYPE",          // 38
   "WF_ENDIF",          // 39
   "IN_QT_MACRO",       // 40
   "IN_FCN_CTOR",       // 41                    Issue #2152
   "IN_TRY_BLOCK",      // 42                    Issue #1734
   "INCOMPLETE",        // 43
   "WF_IF",             // 44
};


std::string pcf_flags_str(pcf_flags_t flags)
{
   char buffer[64];

   // Generate hex representation first
#ifdef WIN32
   snprintf(buffer, 63, "[");
#else // not WIN32
   snprintf(buffer, 63, "[0x%llx:", (long long unsigned int)(flags));
#endif // ifdef WIN32

   // Add human-readable names
   auto out   = std::string{ buffer };
   auto first = true;

   for (size_t i = 0; i < ARRAY_SIZE(pcf_names); ++i)
   {
      if (flags & static_cast<pcf_flag_e>(pcf_bit(i)))
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


void log_pcf_flags(log_sev_t sev, pcf_flags_t flags)
{
   if (!log_sev_on(sev))
   {
      return;
   }
   log_fmt(sev, "%s\n", pcf_flags_str(flags).c_str());
}
