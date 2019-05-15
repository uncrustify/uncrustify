/**
 * @file align.cpp
 * Does all the aligning stuff.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "align.h"

#include "align_asm_colon.h"
#include "align_assign.h"
#include "align_func_params.h"
#include "align_func_proto.h"
#include "align_init_brace.h"
#include "align_left_shift.h"
#include "align_oc_decl_colon.h"
#include "align_oc_msg_colons.h"
#include "align_oc_msg_spec.h"
#include "align_preprocessor.h"
#include "align_same_func_call_params.h"
#include "align_stack.h"
#include "align_struct_initializers.h"
#include "align_trailing_comments.h"
#include "align_typedefs.h"
#include "align_var_def_brace.h"
#include "language_tools.h"
#include "quick_align_again.h"
#include "uncrustify.h"


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
 *        { AD_SW_ERROR,    "A software error has occured.", "Bye!", NULL, NULL },
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
void align_all(void)
{
   LOG_FUNC_ENTRY();
   if (options::align_typedef_span() > 0)
   {
      align_typedefs(options::align_typedef_span());
   }

   if (options::align_left_shift())
   {
      align_left_shift();
   }

   if (options::align_oc_msg_colon_span() > 0)
   {
      align_oc_msg_colons();
   }

   // Align variable definitions
   if (  (options::align_var_def_span() > 0)
      || (options::align_var_struct_span() > 0)
      || (options::align_var_class_span() > 0))
   {
      align_var_def_brace(chunk_get_head(), options::align_var_def_span(), nullptr);
   }

   // Align assignments
   if (  (options::align_enum_equ_span() > 0)
      || (options::align_assign_span() > 0))
   {
      align_assign(chunk_get_head(),
                   options::align_assign_span(),
                   options::align_assign_thresh(),
                   nullptr);
   }

   // Align structure initializers
   if (options::align_struct_init_span() > 0)
   {
      align_struct_initializers();
   }

   // Align function prototypes
   if (  (options::align_func_proto_span() > 0)
      && !options::align_mix_var_proto())
   {
      align_func_proto(options::align_func_proto_span());
   }

   // Align function prototypes
   if (options::align_oc_msg_spec_span() > 0)
   {
      align_oc_msg_spec(options::align_oc_msg_spec_span());
   }

   // Align OC colons
   if (options::align_oc_decl_colon())
   {
      align_oc_decl_colon();
   }

   if (options::align_asm_colon())
   {
      align_asm_colon();
   }

   // Align variable definitions in function prototypes
   if (  options::align_func_params()
      || options::align_func_params_span() > 0)
   {
      align_func_params();
   }

   if (options::align_same_func_call_params())
   {
      align_same_func_call_params();
   }
   // Just in case something was aligned out of order... do it again
   quick_align_again();
} // align_all
