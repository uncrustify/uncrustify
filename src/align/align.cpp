/**
 * @file align.cpp
 * Does all the aligning stuff.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "align/align.h"

#include "align/asm_colon.h"
#include "align/assign.h"
#include "align/braced_init_list.h"
#include "align/eigen_comma_init.h"
#include "align/func_params.h"
#include "align/func_proto.h"
#include "align/init_brace.h"
#include "align/left_shift.h"
#include "align/oc_decl_colon.h"
#include "align/oc_msg_colons.h"
#include "align/oc_msg_spec.h"
#include "align/quick_align_again.h"
#include "align/same_func_call_params.h"
#include "align/struct_initializers.h"
#include "align/trailing_comments.h"
#include "align/typedefs.h"
#include "align/var_def_brace.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALIGN;


using namespace uncrustify;


/*
 *   Here are the items aligned:
 *
 *   - enum value assignments
 *     enum {
 *        cat  = 1,
 *        fred = 2,
 *     };
 *
 *   - struct/union variable & bit definitions
 *     struct foo {
 *        char cat;
 *        int  id       : 5;
 *        int  name_len : 6;
 *        int  height   : 12;
 *     };
 *
 *   - variable definitions & assignments in normal code
 *     const char *cat = "feline";
 *     int        id   = 4;
 *     a   = 5;
 *     bat = 14;
 *
 *   - simple array initializers
 *     int a[] = {
 *        1, 2, 3, 4, 5,
 *        6, 7, 8, 9, 10
 *     };
 *
 *   - c99 array initializers
 *     const char *name[] = {
 *        [FRED]  = "fred",
 *        [JOE]   = "joe",
 *        [PETER] = "peter",
 *     };
 *     struct foo b[] = {
 *        { .id = 1,   .name = "text 1" },
 *        { .id = 567, .name = "text 2" },
 *     };
 *     struct foo_t bars[] =
 *     {
 *        [0] = { .name = "bar",
 *                .age  = 21 },
 *        [1] = { .name = "barley",
 *                .age  = 55 },
 *     };
 *
 *   - compact array initializers
 *     struct foo b[] = {
 *        { 3, "dog" },      { 6, "spider" },
 *        { 8, "elephant" }, { 3, "cat" },
 *     };
 *
 *   - multiline array initializers (2nd line indented, not aligned)
 *     struct foo b[] = {
 *        { AD_NOT_ALLOWED, "Sorry, you failed to guess the password.",
 *          "Try again?", "Yes", "No" },
 *        { AD_SW_ERROR,    "A software error has occurred.", "Bye!", NULL, NULL },
 *     };
 *
 *   - Trailing comments
 *
 *   - Back-slash newline groups
 *
 *   - Function prototypes
 *     int  foo();
 *     void bar();
 *
 *   - Preprocessors
 *     #define FOO_VAL        15
 *     #define MAX_TIMEOUT    60
 *     #define FOO(x)         ((x) * 65)
 *
 *   - typedefs
 *     typedef uint8_t     BYTE;
 *     typedef int32_t     INT32;
 *     typedef uint32_t    UINT32;
 */
void align_all()
{
   LOG_FUNC_ENTRY();

   if (options::align_typedef_span() > 0)
   {
      log_rule_B("align_typedef_span");
      align_typedefs(options::align_typedef_span());
   }

   if (options::align_left_shift())
   {
      log_rule_B("align_left_shift");
      align_left_shift();
   }

   if (options::align_eigen_comma_init())
   {
      log_rule_B("align_eigen_comma_init");
      align_eigen_comma_init();
   }

   if (options::align_oc_msg_colon_span() > 0)
   {
      log_rule_B("align_oc_msg_colon_span");
      align_oc_msg_colons();
   }

   if (  (options::align_var_def_span() > 0)
      || (options::align_var_struct_span() > 0)
      || (options::align_var_class_span() > 0))
   {
      // Align variable definitions
      log_rule_B("align_var_def_span");
      log_rule_B("align_var_struct_span");
      log_rule_B("align_var_class_span");
      align_var_def_brace(Chunk::GetHead(), options::align_var_def_span(), nullptr);
   }

   if (  (options::align_enum_equ_span() > 0)
      || (options::align_assign_span() > 0))
   {
      // Align assignments
      log_rule_B("align_enum_equ_span");
      log_rule_B("align_assign_span");
      align_assign(Chunk::GetHead(),
                   options::align_assign_span(),
                   options::align_assign_thresh(),
                   nullptr);
   }

   if (  (options::align_braced_init_list_span() > 0)                   // Issue #750
      || (options::align_braced_init_list_thresh() > 0))
   {
      // Align braced initializers lists
      align_braced_init_list(Chunk::GetHead(),
                             options::align_braced_init_list_span(),
                             options::align_braced_init_list_thresh(),
                             nullptr);
   }

   if (options::align_struct_init_span() > 0)
   {
      // Align structure initializers
      log_rule_B("align_struct_init_span");
      align_struct_initializers();
   }

   if (  (options::align_func_proto_span() > 0)
      && !options::align_mix_var_proto())
   {
      // Align function prototypes
      log_rule_B("align_func_proto_span");
      log_rule_B("align_mix_var_proto");
      align_func_proto(options::align_func_proto_span());
   }

   if (options::align_oc_msg_spec_span() > 0)
   {
      // Align OC message spec
      log_rule_B("align_oc_msg_spec_span");
      align_oc_msg_spec(options::align_oc_msg_spec_span());
   }

   if (options::align_oc_decl_colon())
   {
      // Align OC colons
      log_rule_B("align_oc_decl_colon");
      align_oc_decl_colon();
   }

   if (options::align_asm_colon())
   {
      // Align ASM colons
      log_rule_B("align_asm_colon");
      align_asm_colon();
   }

   if (  options::align_func_params()
      || options::align_func_params_span() > 0)
   {
      // Align variable definitions in function prototypes
      log_rule_B("align_func_params");
      log_rule_B("align_func_params_span");
      align_func_params();
   }

   if (options::align_same_func_call_params())
   {
      // Align parameters in function call
      log_rule_B("align_same_func_call_params");
      align_same_func_call_params();
   }
   // Just in case something was aligned out of order... do it again
   quick_align_again();
} // align_all
