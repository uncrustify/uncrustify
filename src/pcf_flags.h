/**
 * @file pcf_flags.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef PCF_FLAGS_STR_INCLUDED
#define PCF_FLAGS_STR_INCLUDED

#include "base_types.h"
#include "enum_flags.h"
#include "logger.h"


constexpr auto pcf_bit(size_t b) -> decltype(0ULL)
{
   return(1ULL << b);
}

enum E_PcfFlag : decltype ( 0ULL )
{
// Copy flags are in the lower 17 bits
   PCF_NONE            = 0ULL,
   PCF_COPY_FLAGS      = 0x0001ffffULL,
   PCF_IN_PREPROC      = pcf_bit(0),  //! in a preprocessor
   PCF_IN_STRUCT       = pcf_bit(1),  //! in a struct
   PCF_IN_ENUM         = pcf_bit(2),  //! in enum
   PCF_IN_FCN_DEF      = pcf_bit(3),  //! inside function def parens
   PCF_IN_FCN_CALL     = pcf_bit(4),  //! inside function call parens
   PCF_IN_SPAREN       = pcf_bit(5),  //! inside for/if/while/switch parens
   PCF_IN_TEMPLATE     = pcf_bit(6),
   PCF_IN_TYPEDEF      = pcf_bit(7),
   PCF_IN_CONST_ARGS   = pcf_bit(8),
   PCF_IN_ARRAY_ASSIGN = pcf_bit(9),
   PCF_IN_CLASS        = pcf_bit(10),
   PCF_IN_CLASS_BASE   = pcf_bit(11),
   PCF_IN_NAMESPACE    = pcf_bit(12),
   PCF_IN_FOR          = pcf_bit(13),
   PCF_IN_OC_MSG       = pcf_bit(14),
   PCF_IN_WHERE_SPEC   = pcf_bit(15),  /* inside C# 'where' constraint clause on class or function def */
   PCF_IN_DECLTYPE     = pcf_bit(16),

// Non-Copy flags are in the upper 47 bits
   PCF_FORCE_SPACE     = pcf_bit(17),  //! must have a space after this token
   PCF_STMT_START      = pcf_bit(18),  //! marks the start of a statement
   PCF_EXPR_START      = pcf_bit(19),
   PCF_DONT_INDENT     = pcf_bit(20),  //! already aligned!
   PCF_ALIGN_START     = pcf_bit(21),
   PCF_WAS_ALIGNED     = pcf_bit(22),
   PCF_VAR_TYPE        = pcf_bit(23),  //! part of a variable def type
   PCF_VAR_DEF         = pcf_bit(24),  //! variable name in a variable def
   PCF_VAR_1ST         = pcf_bit(25),  //! 1st variable def in a statement
   PCF_VAR_1ST_DEF     = (PCF_VAR_DEF | PCF_VAR_1ST),
   PCF_VAR_INLINE      = pcf_bit(26),  //! type was an inline struct/enum/union
   PCF_RIGHT_COMMENT   = pcf_bit(27),
   PCF_OLD_FCN_PARAMS  = pcf_bit(28),
   PCF_LVALUE          = pcf_bit(29),  //! left of assignment
   PCF_ONE_LINER       = pcf_bit(30),
   PCF_ONE_CLASS       = (PCF_ONE_LINER | PCF_IN_CLASS),
   PCF_EMPTY_BODY      = pcf_bit(31),
   PCF_ANCHOR          = pcf_bit(32),  //! aligning anchor
   PCF_PUNCTUATOR      = pcf_bit(33),
   PCF_INSERTED        = pcf_bit(34),  //! chunk was inserted from another file
   PCF_LONG_BLOCK      = pcf_bit(35),  //! the block is 'long' by some measure
   PCF_OC_BOXED        = pcf_bit(36),  //! inside OC boxed expression
   PCF_KEEP_BRACE      = pcf_bit(37),  //! do not remove brace
   PCF_OC_RTYPE        = pcf_bit(38),  //! inside OC return type
   PCF_OC_ATYPE        = pcf_bit(39),  //! inside OC arg type
   PCF_WF_ENDIF        = pcf_bit(40),  //! #endif for whole file ifdef
   PCF_IN_QT_MACRO     = pcf_bit(41),  //! in a QT-macro, i.e. SIGNAL, SLOT
   PCF_IN_FCN_CTOR     = pcf_bit(42),  //! inside function constructor
   PCF_IN_TRY_BLOCK    = pcf_bit(43),  //! inside Function-try-block
   PCF_INCOMPLETE      = pcf_bit(44),  //! class/struct forward declaration
   PCF_IN_LAMBDA       = pcf_bit(45),  //! inside a lambda expression
   PCF_WF_IF           = pcf_bit(46),  //! #if for a whole file ifdef
   PCF_NOT_POSSIBLE    = pcf_bit(47),  //! it is not possible to make an one_liner
                                       //! because the line would be too long
   PCF_IN_CONDITIONAL  = pcf_bit(48),  //! inside a conditional ternary expression
   PCF_OC_IN_BLOCK     = pcf_bit(49),  //! inside OC block function
   PCF_CONT_LINE       = pcf_bit(50),  //! continuation line split
};

UNC_DECLARE_FLAGS(PcfFlags, E_PcfFlag);
UNC_DECLARE_OPERATORS_FOR_FLAGS(PcfFlags);

std::string pcf_flags_str(PcfFlags flags);


void log_pcf_flags(log_sev_t sev, PcfFlags flags);


#endif /* PCF_FLAGS_STR_INCLUDED */
