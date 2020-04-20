/**
 * @file combine.cpp
 * Labels the chunks as needed.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "combine.h"

#include "chunk_list.h"
#include "combine_fix_mark.h"
#include "combine_labels.h"
#include "combine_skip.h"
#include "combine_tools.h"
#include "ChunkStack.h"
#include "error_types.h"
#include "flag_parens.h"
#include "lang_pawn.h"
#include "language_tools.h"
#include "log_rules.h"
#include "newlines.h"
#include "prototypes.h"
#include "tokenize_cleanup.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>

using namespace std;
using namespace uncrustify;


/**
 * Mark the parens and colons in:
 *   asm volatile ( "xx" : "xx" (l), "yy"(h) : ...  );
 *
 * @param pc  the CT_ASM item
 */
static void flag_asm(chunk_t *pc);


/**
 * Skips the list of class/struct parent types.
 */
chunk_t *skip_parent_types(chunk_t *colon);


/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
static void check_double_brace_init(chunk_t *bo1);


static void process_returns(void);


/**
 * Processes a return statement, labeling the parens and marking the parent.
 * May remove or add parens around the return statement
 *
 * @param pc  Pointer to the return chunk
 */
static chunk_t *process_return(chunk_t *pc);


/**
 * Process an ObjC 'class'
 * pc is the chunk after '@implementation' or '@interface' or '@protocol'.
 * Change colons, etc. Processes stuff until '@end'.
 * Skips anything in braces.
 */
static void handle_oc_class(chunk_t *pc);


/**
 *  Mark Objective-C blocks (aka lambdas or closures)
 *  The syntax and usage is exactly like C function pointers
 *  but instead of an asterisk they have a caret as pointer symbol.
 *  Although it may look expensive this functions is only triggered
 *  on appearance of an OC_BLOCK_CARET for LANG_OC.
 *  repeat(10, ^{ putc('0'+d); });
 *  typedef void (^workBlk_t)(void);
 *
 * @param pc  points to the '^'
 */
static void handle_oc_block_literal(chunk_t *pc);


/**
 * Mark Objective-C block types.
 * The syntax and usage is exactly like C function pointers
 * but instead of an asterisk they have a caret as pointer symbol.
 *  typedef void (^workBlk_t)(void);
 *  const char * (^workVar)(void);
 *  -(void)Foo:(void(^)())blk { }
 *
 * This is triggered when the sequence '(' '^' is found.
 *
 * @param pc  points to the '^'
 */
static void handle_oc_block_type(chunk_t *pc);


/**
 * Process an ObjC message spec/dec
 *
 * Specs:
 * -(void) foo ARGS;
 *
 * Declaration:
 * -(void) foo ARGS {  }
 *
 * LABEL : (ARGTYPE) ARGNAME
 *
 * ARGS is ': (ARGTYPE) ARGNAME [MOREARGS...]'
 * MOREARGS is ' [ LABEL] : (ARGTYPE) ARGNAME '
 * -(void) foo: (int) arg: {  }
 * -(void) foo: (int) arg: {  }
 * -(void) insertObject:(id)anObject atIndex:(int)index
 */
static void handle_oc_message_decl(chunk_t *pc);


/**
 * Process an ObjC message send statement:
 * [ class func: val1 name2: val2 name3: val3] ; // named params
 * [ class func: val1      : val2      : val3] ; // unnamed params
 * [ class <proto> self method ] ; // with protocol
 * [[NSMutableString alloc] initWithString: @"" ] // class from msg
 * [func(a,b,c) lastObject ] // class from func
 *
 * Mainly find the matching ']' and ';' and mark the colons.
 *
 * @param pc  points to the open square '['
 */
static void handle_oc_message_send(chunk_t *pc);


//! Process @Property values and re-arrange them if necessary
static void handle_oc_property_decl(chunk_t *pc);

//! Process @available annotation
static void handle_oc_available(chunk_t *pc);

/**
 * Process a type that is enclosed in parens in message declarations.
 * TODO: handle block types, which get special formatting
 *
 * @param pc  points to the open paren
 *
 * @return the chunk after the type
 */
static chunk_t *handle_oc_md_type(chunk_t *paren_open, c_token_t ptype, pcf_flags_t flags, bool &did_it);

/**
 * Process an C# [] thingy:
 *    [assembly: xxx]
 *    [AttributeUsage()]
 *    [@X]
 *
 * Set the next chunk to a statement start after the close ']'
 *
 * @param pc  points to the open square '['
 */
static void handle_cs_square_stmt(chunk_t *pc);


/**
 * We are on a brace open that is preceded by a word or square close.
 * Set the brace parent to CT_CS_PROPERTY and find the first item in the
 * property and set its parent, too.
 */
static void handle_cs_property(chunk_t *pc);


/**
 * We hit a ']' followed by a WORD. This may be a multidimensional array type.
 * Example: int[,,] x;
 * If there is nothing but commas between the open and close, then mark it.
 */
static void handle_cs_array_type(chunk_t *pc);


/**
 * We are on the C++ 'template' keyword.
 * What follows should be the following:
 *
 * template <class identifier> function_declaration;
 * template <typename identifier> function_declaration;
 * template <class identifier> class class_declaration;
 * template <typename identifier> class class_declaration;
 *
 * Change the 'class' inside the <> to CT_TYPE.
 * Set the parent to the class after the <> to CT_TEMPLATE.
 * Set the parent of the semicolon to CT_TEMPLATE.
 */
static void handle_cpp_template(chunk_t *pc);


/**
 * Verify and then mark C++ lambda expressions.
 * The expected format is '[...](...){...}' or '[...](...) -> type {...}'
 * sq_o is '[' CT_SQUARE_OPEN or '[]' CT_TSQUARE
 * Split the '[]' so we can control the space
 */
static void handle_cpp_lambda(chunk_t *pc);


/**
 * We are on the D 'template' keyword.
 * What follows should be the following:
 *
 * template NAME ( TYPELIST ) { BODY }
 *
 * Set the parent of NAME to template, change NAME to CT_TYPE.
 * Set the parent of the parens and braces to CT_TEMPLATE.
 * Scan the body for each type in TYPELIST and change the type to CT_TYPE.
 */
static void handle_d_template(chunk_t *pc);


/**
 * A func wrap chunk and what follows should be treated as a function name.
 * Create new text for the chunk and call it a CT_FUNCTION.
 *
 * A type wrap chunk and what follows should be treated as a simple type.
 * Create new text for the chunk and call it a CT_TYPE.
 */
static void handle_wrap(chunk_t *pc);


/**
 * A proto wrap chunk and what follows should be treated as a function proto.
 *
 * RETTYPE PROTO_WRAP( NAME, PARAMS ); or RETTYPE PROTO_WRAP( NAME, (PARAMS) );
 * RETTYPE gets changed with make_type().
 * PROTO_WRAP is marked as CT_FUNC_PROTO or CT_FUNC_DEF.
 * NAME is marked as CT_WORD.
 * PARAMS is all marked as prototype parameters.
 */
static void handle_proto_wrap(chunk_t *pc);


static bool is_oc_block(chunk_t *pc);


/**
 * Java assert statements are: "assert EXP1 [: EXP2] ;"
 * Mark the parent of the colon and semicolon
 */
static void handle_java_assert(chunk_t *pc);


static void flag_asm(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   chunk_t *tmp = chunk_get_next_ncnl(pc, scope_e::PREPROC);

   if (!chunk_is_token(tmp, CT_QUALIFIER))
   {
      return;
   }
   chunk_t *po = chunk_get_next_ncnl(tmp, scope_e::PREPROC);

   if (!chunk_is_paren_open(po))
   {
      return;
   }
   chunk_t *end = chunk_skip_to_match(po, scope_e::PREPROC);

   if (end == nullptr)
   {
      return;
   }
   set_chunk_parent(po, CT_ASM);
   set_chunk_parent(end, CT_ASM);

   for (  tmp = chunk_get_next_ncnl(po, scope_e::PREPROC);
          tmp != nullptr
       && tmp != end;
          tmp = chunk_get_next_ncnl(tmp, scope_e::PREPROC))
   {
      if (chunk_is_token(tmp, CT_COLON))
      {
         set_chunk_type(tmp, CT_ASM_COLON);
      }
      else if (chunk_is_token(tmp, CT_DC_MEMBER))
      {
         // if there is a string on both sides, then this is two ASM_COLONs
         if (  chunk_is_token(chunk_get_next_ncnl(tmp, scope_e::PREPROC), CT_STRING)
            && chunk_is_token(chunk_get_prev_ncnlni(tmp, scope_e::PREPROC), CT_STRING)) // Issue #2279
         {
            chunk_t nc;

            nc = *tmp;

            tmp->str.resize(1);
            tmp->orig_col_end = tmp->orig_col + 1;
            set_chunk_type(tmp, CT_ASM_COLON);

            set_chunk_type(&nc, tmp->type);
            nc.str.pop_front();
            nc.orig_col++;
            nc.column++;
            chunk_add_after(&nc, tmp);
         }
      }
   }

   tmp = chunk_get_next_ncnl(end, scope_e::PREPROC);

   if (tmp == nullptr)
   {
      return;
   }

   if (chunk_is_token(tmp, CT_SEMICOLON))
   {
      set_chunk_parent(tmp, CT_ASM);
   }
} // flag_asm


void do_symbol_check(chunk_t *prev, chunk_t *pc, chunk_t *next)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   // separate the uses of CT_ASSIGN sign '='
   // into CT_ASSIGN_DEFAULT_ARG, CT_ASSIGN_FUNC_PROTO
   if (  chunk_is_token(pc, CT_ASSIGN)
      && get_chunk_parent_type(pc) == CT_FUNC_PROTO
      && (  pc->flags.test(PCF_IN_FCN_DEF)                            // Issue #2236
         || pc->flags.test(PCF_IN_CONST_ARGS)))
   {
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      log_pcf_flags(LFCNR, pc->flags);
      set_chunk_type(pc, CT_ASSIGN_DEFAULT_ARG);
   }

   if (  (  chunk_is_token(prev, CT_FPAREN_CLOSE)
         || (  (  chunk_is_str(prev, "const", 5)
               || chunk_is_str(prev, "override", 8))
            && chunk_is_token(prev->prev, CT_FPAREN_CLOSE)))
      && chunk_is_token(pc, CT_ASSIGN)
      && (  chunk_is_token(next, CT_DEFAULT)
         || chunk_is_token(next, CT_DELETE)
         || chunk_is_str(next, "0", 1)))
   {
      set_chunk_type(pc, CT_ASSIGN_FUNC_PROTO);
   }

   if (chunk_is_token(pc, CT_OC_AT))
   {
      if (  chunk_is_token(next, CT_PAREN_OPEN)
         || chunk_is_token(next, CT_BRACE_OPEN)
         || chunk_is_token(next, CT_SQUARE_OPEN))
      {
         flag_parens(next, PCF_OC_BOXED, next->type, CT_OC_AT, false);
      }
      else
      {
         set_chunk_parent(next, CT_OC_AT);
      }
   }

   // D stuff
   if (  language_is_set(LANG_D)
      && chunk_is_token(pc, CT_QUALIFIER)
      && chunk_is_str(pc, "const", 5)
      && chunk_is_token(next, CT_PAREN_OPEN))
   {
      set_chunk_type(pc, CT_D_CAST);
      set_paren_parent(next, pc->type);
   }

   if (  chunk_is_token(next, CT_PAREN_OPEN)
      && (  chunk_is_token(pc, CT_D_CAST)
         || chunk_is_token(pc, CT_DELEGATE)
         || chunk_is_token(pc, CT_ALIGN)))
   {
      // mark the parenthesis parent
      tmp = set_paren_parent(next, pc->type);

      // For a D cast - convert the next item
      if (  chunk_is_token(pc, CT_D_CAST)
         && tmp != nullptr)
      {
         if (chunk_is_token(tmp, CT_STAR))
         {
            set_chunk_type(tmp, CT_DEREF);
         }
         else if (chunk_is_token(tmp, CT_AMP))
         {
            set_chunk_type(tmp, CT_ADDR);
         }
         else if (chunk_is_token(tmp, CT_MINUS))
         {
            set_chunk_type(tmp, CT_NEG);
         }
         else if (chunk_is_token(tmp, CT_PLUS))
         {
            set_chunk_type(tmp, CT_POS);
         }
      }

      /*
       * For a delegate, mark previous words as types and the item after the
       * close paren as a variable def
       */
      if (chunk_is_token(pc, CT_DELEGATE))
      {
         if (tmp != nullptr)
         {
            set_chunk_parent(tmp, CT_DELEGATE);

            if (tmp->level == tmp->brace_level)
            {
               chunk_flags_set(tmp, PCF_VAR_1ST_DEF);
            }
         }

         for (tmp = chunk_get_prev_ncnlni(pc); tmp != nullptr; tmp = chunk_get_prev_ncnlni(tmp)) // Issue #2279
         {
            if (  chunk_is_semicolon(tmp)
               || chunk_is_token(tmp, CT_BRACE_OPEN)
               || chunk_is_token(tmp, CT_VBRACE_OPEN))
            {
               break;
            }
            make_type(tmp);
         }
      }

      if (  chunk_is_token(pc, CT_ALIGN)
         && tmp != nullptr)
      {
         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, pc->type);
         }
         else if (chunk_is_token(tmp, CT_COLON))
         {
            set_chunk_parent(tmp, pc->type);
         }
      }
   } // paren open + cast/align/delegate

   if (chunk_is_token(pc, CT_INVARIANT))
   {
      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_chunk_parent(next, pc->type);
         tmp = chunk_get_next(next);

         while (tmp != nullptr)
         {
            if (chunk_is_token(tmp, CT_PAREN_CLOSE))
            {
               set_chunk_parent(tmp, pc->type);
               break;
            }
            make_type(tmp);
            tmp = chunk_get_next(tmp);
         }
      }
      else
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }
   }

   if (  chunk_is_token(prev, CT_BRACE_OPEN)
      && get_chunk_parent_type(prev) != CT_CS_PROPERTY
      && (  chunk_is_token(pc, CT_GETSET)
         || chunk_is_token(pc, CT_GETSET_EMPTY)))
   {
      flag_parens(prev, PCF_NONE, CT_NONE, CT_GETSET, false);
   }

   if (chunk_is_token(pc, CT_ASM))
   {
      flag_asm(pc);
   }

   // clang stuff - A new derived type is introduced to C and, by extension, Objective-C, C++, and Objective-C++
   if (language_is_set(LANG_C | LANG_CPP | LANG_OC))
   {
      if (chunk_is_token(pc, CT_CARET))
      {
         if (  pc->flags.test(PCF_EXPR_START)
            || pc->flags.test(PCF_IN_PREPROC))
         {
            handle_oc_block_literal(pc);
         }
      }
   }

   // Objective C stuff
   if (language_is_set(LANG_OC))
   {
      // Check for message declarations
      if (pc->flags.test(PCF_STMT_START))
      {
         if (  (  chunk_is_str(pc, "-", 1)
               || chunk_is_str(pc, "+", 1))
            && chunk_is_str(next, "(", 1))
         {
            handle_oc_message_decl(pc);
         }
      }

      if (  pc->flags.test(PCF_EXPR_START)
         || pc->flags.test(PCF_IN_PREPROC))
      {
         if (chunk_is_token(pc, CT_SQUARE_OPEN))
         {
            handle_oc_message_send(pc);
         }
      }

      if (chunk_is_token(pc, CT_OC_PROPERTY))
      {
         handle_oc_property_decl(pc);
      }

      if (chunk_is_token(pc, CT_OC_AVAILABLE))
      {
         handle_oc_available(pc);
      }
   }

   // C# stuff
   if (language_is_set(LANG_CS))
   {
      // '[assembly: xxx]' stuff
      if (  pc->flags.test(PCF_EXPR_START)
         && chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         handle_cs_square_stmt(pc);
      }

      if (  chunk_is_token(next, CT_BRACE_OPEN)
         && get_chunk_parent_type(next) == CT_NONE
         && (  chunk_is_token(pc, CT_SQUARE_CLOSE)
            || chunk_is_token(pc, CT_ANGLE_CLOSE)
            || chunk_is_token(pc, CT_WORD)))
      {
         handle_cs_property(next);
      }

      if (  chunk_is_token(pc, CT_SQUARE_CLOSE)
         && chunk_is_token(next, CT_WORD))
      {
         handle_cs_array_type(pc);
      }

      if (  (  chunk_is_token(pc, CT_LAMBDA)
            || chunk_is_token(pc, CT_DELEGATE))
         && chunk_is_token(next, CT_BRACE_OPEN))
      {
         set_paren_parent(next, pc->type);
      }

      if (chunk_is_token(pc, CT_WHEN) && pc->next->type != CT_SPAREN_OPEN)
      {
         set_chunk_type(pc, CT_WORD);
      }
   }

   if (  language_is_set(LANG_JAVA)
      && chunk_is_token(pc, CT_LAMBDA)
      && chunk_is_token(next, CT_BRACE_OPEN))
   {
      set_paren_parent(next, pc->type);
   }

   if (chunk_is_token(pc, CT_NEW))
   {
      chunk_t *ts = nullptr;
      tmp = next;

      if (chunk_is_token(tmp, CT_TSQUARE))
      {
         ts  = tmp;
         tmp = chunk_get_next_ncnl(tmp);
      }

      if (  chunk_is_token(tmp, CT_BRACE_OPEN)
         || chunk_is_token(tmp, CT_PAREN_OPEN))
      {
         set_paren_parent(tmp, pc->type);

         if (ts != nullptr)
         {
            set_chunk_parent(ts, pc->type);
         }
      }
   }

   // C++11 Lambda stuff
   if (  language_is_set(LANG_CPP)
      && (  chunk_is_token(pc, CT_SQUARE_OPEN)
         || chunk_is_token(pc, CT_TSQUARE)))
   {
      handle_cpp_lambda(pc);
   }

   // FIXME: which language does this apply to?
   // Issue #2432
   if (!language_is_set(LANG_OC))
   {
      if (  chunk_is_token(pc, CT_ASSIGN)
         && chunk_is_token(next, CT_SQUARE_OPEN))
      {
         set_paren_parent(next, CT_ASSIGN);

         // Mark one-liner assignment
         tmp = next;

         while ((tmp = chunk_get_next_nc(tmp)) != nullptr)
         {
            if (chunk_is_newline(tmp))
            {
               break;
            }

            if (chunk_is_token(tmp, CT_SQUARE_CLOSE) && next->level == tmp->level)
            {
               chunk_flags_set(tmp, PCF_ONE_LINER);
               chunk_flags_set(next, PCF_ONE_LINER);
               break;
            }
         }
      }
   }

   if (chunk_is_token(pc, CT_ASSERT))
   {
      handle_java_assert(pc);
   }

   if (chunk_is_token(pc, CT_ANNOTATION))
   {
      tmp = chunk_get_next_ncnl(pc);

      if (chunk_is_paren_open(tmp))
      {
         set_paren_parent(tmp, CT_ANNOTATION);
      }
   }

   if (chunk_is_token(pc, CT_SIZEOF) && language_is_set(LANG_ALLC))
   {
      tmp = chunk_get_next_ncnl(pc);

      if (chunk_is_token(tmp, CT_ELLIPSIS))
      {
         set_chunk_parent(tmp, CT_SIZEOF);
      }
   }

   if (chunk_is_token(pc, CT_DECLTYPE))
   {
      tmp = chunk_get_next_ncnl(pc);

      if (chunk_is_paren_open(tmp))
      {
         // decltype may be followed by a braced-init-list
         tmp = set_paren_parent(tmp, CT_DECLTYPE);

         if (chunk_is_opening_brace(tmp))
         {
            tmp = set_paren_parent(tmp, CT_BRACED_INIT_LIST);

            if (tmp)
            {
               chunk_flags_clr(tmp, PCF_EXPR_START | PCF_STMT_START);
            }
         }
         else
         {
            if (chunk_is_token(tmp, CT_WORD))
            {
               chunk_flags_set(tmp, PCF_VAR_1ST_DEF);
            }
         }
      }
   }

   // A [] in C# and D only follows a type
   if (  chunk_is_token(pc, CT_TSQUARE)
      && language_is_set(LANG_D | LANG_CS | LANG_VALA))
   {
      if (chunk_is_token(prev, CT_WORD))
      {
         set_chunk_type(prev, CT_TYPE);
      }

      if (chunk_is_token(next, CT_WORD))
      {
         chunk_flags_set(next, PCF_VAR_1ST_DEF);
      }
   }

   if (  chunk_is_token(pc, CT_SQL_EXEC)
      || chunk_is_token(pc, CT_SQL_BEGIN)
      || chunk_is_token(pc, CT_SQL_END))
   {
      mark_exec_sql(pc);
   }

   if (chunk_is_token(pc, CT_PROTO_WRAP))
   {
      handle_proto_wrap(pc);
   }

   // Handle the typedef
   if (chunk_is_token(pc, CT_TYPEDEF))
   {
      fix_typedef(pc);
   }

   if (  chunk_is_token(pc, CT_ENUM)
      || chunk_is_token(pc, CT_STRUCT)
      || chunk_is_token(pc, CT_UNION)
      || (  chunk_is_token(pc, CT_CLASS)
         && !language_is_set(LANG_D)))
   {
      if (prev->type != CT_TYPEDEF)
      {
         fix_enum_struct_union(pc);
      }
   }

   if (chunk_is_token(pc, CT_EXTERN))
   {
      if (chunk_is_paren_open(next))
      {
         tmp = flag_parens(next, PCF_NONE, CT_NONE, CT_EXTERN, true);

         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, CT_EXTERN);
         }
      }
      else
      {
         // next likely is a string (see tokenize_cleanup.cpp)
         set_chunk_parent(next, CT_EXTERN);
         tmp = chunk_get_next_ncnl(next);

         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, CT_EXTERN);
         }
      }
   }

   if (chunk_is_token(pc, CT_TEMPLATE))
   {
      if (language_is_set(LANG_D))
      {
         handle_d_template(pc);
      }
      else
      {
         handle_cpp_template(pc);
      }
   }

   if (  chunk_is_token(pc, CT_WORD)
      && chunk_is_token(next, CT_ANGLE_OPEN)
      && get_chunk_parent_type(next) == CT_TEMPLATE)
   {
      mark_template_func(pc, next);
   }

   if (  chunk_is_token(pc, CT_SQUARE_CLOSE)
      && chunk_is_token(next, CT_PAREN_OPEN))
   {
      flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, CT_NONE, false);
   }

   if (chunk_is_token(pc, CT_TYPE_CAST))
   {
      fix_type_cast(pc);
   }

   if (  get_chunk_parent_type(pc) == CT_ASSIGN
      && (chunk_is_token(pc, CT_BRACE_OPEN) || chunk_is_token(pc, CT_SQUARE_OPEN)))
   {
      // Mark everything in here as in assign
      flag_parens(pc, PCF_IN_ARRAY_ASSIGN, pc->type, CT_NONE, false);
   }

   if (chunk_is_token(pc, CT_D_TEMPLATE))
   {
      set_paren_parent(next, pc->type);
   }

   /*
    * A word before an open paren is a function call or definition.
    * CT_WORD => CT_FUNC_CALL or CT_FUNC_DEF
    */
   if (chunk_is_token(next, CT_PAREN_OPEN))
   {
      tmp = chunk_get_next_ncnl(next);

      if (  language_is_set(LANG_C | LANG_CPP | LANG_OC)
         && chunk_is_token(tmp, CT_CARET))
      {
         handle_oc_block_type(tmp);

         // This is the case where a block literal is passed as the first argument of a C-style method invocation.
         if (  (  chunk_is_token(tmp, CT_OC_BLOCK_CARET)
               || chunk_is_token(tmp, CT_CARET))
            && chunk_is_token(pc, CT_WORD))
         {
            LOG_FMT(LFCN, "%s(%d): (1) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
            set_chunk_type(pc, CT_FUNC_CALL);
         }
      }
      else if (  chunk_is_token(pc, CT_WORD)
              || chunk_is_token(pc, CT_OPERATOR_VAL))
      {
         set_chunk_type(pc, CT_FUNCTION);
      }
      else if (chunk_is_token(pc, CT_FIXED))
      {
         set_chunk_type(pc, CT_FUNCTION);
         set_chunk_parent(pc, CT_FIXED);
      }
      else if (chunk_is_token(pc, CT_TYPE))
      {
         /*
          * If we are on a type, then we are either on a C++ style cast, an
          * array reference, a function or we are on a function type.
          * The only way to tell for sure is to find the close paren and see
          * if it is followed by an open paren.
          * "int(5.6)"
          * "int()"
          * "int(foo)(void)"
          *
          * FIXME: this check can be done better...
          */
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

         bool is_byref_array = false;

         if (language_is_set(LANG_CPP))
         {
            // If the open paren is followed by an ampersand, an optional word,
            // a close parenthesis, and an open square bracket, then it is an
            // array being passed by reference, not a cast
            tmp = chunk_get_next_ncnl(next);

            if (chunk_is_token(tmp, CT_AMP))
            {
               auto tmp2 = chunk_get_next_ncnl(tmp);

               if (chunk_is_token(tmp2, CT_WORD))
               {
                  tmp2 = chunk_get_next_ncnl(tmp2);
               }

               if (chunk_is_token(tmp2, CT_PAREN_CLOSE))
               {
                  tmp2 = chunk_get_next_ncnl(tmp2);

                  if (chunk_is_token(tmp2, CT_SQUARE_OPEN))
                  {
                     is_byref_array = true;
                     set_chunk_type(tmp, CT_BYREF);
                  }
               }
            }
         }

         if (!is_byref_array)
         {
            tmp = chunk_get_next_type(next, CT_PAREN_CLOSE, next->level);

            if (tmp != nullptr)
            {
               tmp = chunk_get_next(tmp);

               if (chunk_is_token(tmp, CT_PAREN_OPEN))
               {
                  set_chunk_type(pc, CT_FUNCTION);
               }
               else
               {
                  if (  get_chunk_parent_type(pc) == CT_NONE
                     && !pc->flags.test(PCF_IN_TYPEDEF))
                  {
                     tmp = chunk_get_next_ncnl(next);

                     if (chunk_is_token(tmp, CT_PAREN_CLOSE))
                     {
                        // we have TYPE()
                        set_chunk_type(pc, CT_FUNCTION);
                     }
                     else
                     {
                        // we have TYPE(...)
                        set_chunk_type(pc, CT_CPP_CAST);
                        set_paren_parent(next, CT_CPP_CAST);
                     }
                  }
               }
            }
         }
      }
   }

   if (language_is_set(LANG_PAWN))
   {
      if (  chunk_is_token(pc, CT_FUNCTION)
         && pc->brace_level > 0)
      {
         LOG_FMT(LFCN, "%s(%d): (2) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         set_chunk_type(pc, CT_FUNC_CALL);
      }

      if (  chunk_is_token(pc, CT_STATE)
         && chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_paren_parent(next, pc->type);
      }
   }
   else
   {
      if (  (  chunk_is_token(pc, CT_FUNCTION)
            || chunk_is_token(pc, CT_FUNC_DEF))
         && (  (get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR)
            || !is_oc_block(pc)))
      {
         mark_function(pc);
      }
   }

   // Detect C99 member stuff
   if (  chunk_is_token(pc, CT_MEMBER)
      && (  chunk_is_token(prev, CT_COMMA)
         || chunk_is_token(prev, CT_BRACE_OPEN)))
   {
      set_chunk_type(pc, CT_C99_MEMBER);
      set_chunk_parent(next, CT_C99_MEMBER);
   }

   // Mark function parens and braces
   if (  chunk_is_token(pc, CT_FUNC_DEF)
      || chunk_is_token(pc, CT_FUNC_CALL)
      || chunk_is_token(pc, CT_FUNC_CALL_USER)
      || chunk_is_token(pc, CT_FUNC_PROTO))
   {
      tmp = next;

      if (chunk_is_token(tmp, CT_SQUARE_OPEN))
      {
         tmp = set_paren_parent(tmp, pc->type);
      }
      else if (chunk_is_token(tmp, CT_TSQUARE) || get_chunk_parent_type(tmp) == CT_OPERATOR)
      {
         tmp = chunk_get_next_ncnl(tmp);
      }

      if (tmp != nullptr)
      {
         if (chunk_is_paren_open(tmp))
         {
            tmp = flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, pc->type, false);

            if (tmp != nullptr)
            {
               if (chunk_is_token(tmp, CT_BRACE_OPEN))
               {
                  if (  get_chunk_parent_type(tmp) != CT_DOUBLE_BRACE
                     && !pc->flags.test(PCF_IN_CONST_ARGS))
                  {
                     set_paren_parent(tmp, pc->type);
                  }
               }
               else if (  chunk_is_semicolon(tmp)
                       && chunk_is_token(pc, CT_FUNC_PROTO))
               {
                  set_chunk_parent(tmp, pc->type);
               }
            }
         }
      }
   }

   // Mark the parameters in catch()
   if (chunk_is_token(pc, CT_CATCH) && chunk_is_token(next, CT_SPAREN_OPEN))
   {
      fix_fcn_def_params(next);
   }

   if (chunk_is_token(pc, CT_THROW) && chunk_is_token(prev, CT_FPAREN_CLOSE))
   {
      set_chunk_parent(pc, get_chunk_parent_type(prev));

      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_paren_parent(next, CT_THROW);
      }
   }

   // Mark the braces in: "for_each_entry(xxx) { }"
   if (  chunk_is_token(pc, CT_BRACE_OPEN)
      && get_chunk_parent_type(pc) != CT_DOUBLE_BRACE
      && chunk_is_token(prev, CT_FPAREN_CLOSE)
      && (  get_chunk_parent_type(prev) == CT_FUNC_CALL
         || get_chunk_parent_type(prev) == CT_FUNC_CALL_USER)
      && !pc->flags.test(PCF_IN_CONST_ARGS))
   {
      LOG_FMT(LFCN, "%s(%d): (3) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      set_paren_parent(pc, CT_FUNC_CALL);
   }

   /*
    * Check for a close parenthesis followed by an open parenthesis,
    * which means that we are on a function type declaration (C/C++ only?).
    * Note that typedefs are already taken care of.
    */
   if (  !pc->flags.test_any(PCF_IN_TYPEDEF | PCF_IN_TEMPLATE)
      && get_chunk_parent_type(pc) != CT_CPP_CAST
      && get_chunk_parent_type(pc) != CT_C_CAST
      && !pc->flags.test(PCF_IN_PREPROC)
      && !is_oc_block(pc)
      && get_chunk_parent_type(pc) != CT_OC_MSG_DECL
      && get_chunk_parent_type(pc) != CT_OC_MSG_SPEC
      && chunk_is_str(pc, ")", 1)
      && chunk_is_str(next, "(", 1))
   {
      if (language_is_set(LANG_D))
      {
         flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
      }
      else
      {
         mark_function_type(pc);
      }
   }

   if (  (chunk_is_token(pc, CT_CLASS) || chunk_is_token(pc, CT_STRUCT))
      && pc->level == pc->brace_level)
   {
      if (pc->type != CT_STRUCT || !language_is_set(LANG_C))
      {
         mark_class_ctor(pc);
      }
   }

   if (chunk_is_token(pc, CT_OC_CLASS))
   {
      handle_oc_class(pc);
   }
   // TODO: Check for stuff that can only occur at the start of an statement

   if (!language_is_set(LANG_D))
   {
      /*
       * Check a parenthesis pair to see if it is a cast.
       * Note that SPAREN and FPAREN have already been marked.
       */
      if (  chunk_is_token(pc, CT_PAREN_OPEN)
         && (  get_chunk_parent_type(pc) == CT_NONE
            || get_chunk_parent_type(pc) == CT_OC_MSG
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR
            || get_chunk_parent_type(pc) == CT_CS_SQ_STMT)           // Issue # 1256
         && (  chunk_is_token(next, CT_WORD)
            || chunk_is_token(next, CT_TYPE)
            || chunk_is_token(next, CT_STRUCT)
            || chunk_is_token(next, CT_QUALIFIER)
            || chunk_is_token(next, CT_MEMBER)
            || chunk_is_token(next, CT_DC_MEMBER)
            || chunk_is_token(next, CT_ENUM)
            || chunk_is_token(next, CT_UNION))
         && prev->type != CT_DECLTYPE
         && prev->type != CT_SIZEOF
         && get_chunk_parent_type(prev) != CT_SIZEOF
         && get_chunk_parent_type(prev) != CT_OPERATOR
         && !pc->flags.test(PCF_IN_TYPEDEF))
      {
         fix_casts(pc);
      }
   }

   if (language_is_set(LANG_CPP))
   {
      chunk_t *nnext = chunk_get_next_ncnl(next);

      // handle parent_type of assigns in special functions (ro5 + pure virtual)
      if (  pc->flags.test_any(PCF_IN_STRUCT | PCF_IN_CLASS)
         && chunk_is_token(pc, CT_ASSIGN)
         && chunk_is_token(nnext, CT_SEMICOLON)
         && (  chunk_is_token(next, CT_DEFAULT)
            || chunk_is_token(next, CT_DELETE)
            || (chunk_is_token(next, CT_NUMBER) && chunk_is_str(next, "0", 1))))
      {
         const size_t level        = pc->level;
         bool         found_status = false;
         chunk_t      *pprev       = chunk_get_prev(pc);

         for ( ; (  pprev != nullptr
                 && pprev->level >= level
                 && pprev->type != CT_SEMICOLON
                 && pprev->type != CT_ACCESS_COLON)
               ; pprev = chunk_get_prev(pprev))
         {
            if (pprev->level != level)
            {
               continue;
            }

            if (chunk_is_token(next, CT_NUMBER))
            {
               if (  pprev->type == CT_QUALIFIER
                  && chunk_is_str(pprev, "virtual", 7))
               {
                  found_status = true;
                  break;
               }
            }
            else
            {
               if (  pprev->type == CT_FUNC_CLASS_PROTO  // ctor/dtor
                  || pprev->type == CT_FUNC_PROTO)       // normal function
               {
                  found_status = true;
                  break;
               }
            }
         }

         if (found_status)
         {
            set_chunk_parent(pc, pprev->type);
         }
      }
      // Issue #2332
      bool we_have_a_case_before = false;

      if (chunk_is_token(pc, CT_COLON))
      {
         // check if we have a case before
         chunk_t *switch_before = chunk_get_prev_type(pc, CT_CASE, pc->level);

         if (switch_before != nullptr)
         {
            LOG_FMT(LFCNR, "%s(%d): switch_before->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                    __func__, __LINE__, switch_before->orig_line, switch_before->orig_col,
                    switch_before->text(), get_token_name(switch_before->type));
            we_have_a_case_before = true;
         }
      }

      // Detect a braced-init-list
      if (  chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_ASSIGN)
         || chunk_is_token(pc, CT_RETURN)
         || chunk_is_token(pc, CT_COMMA)
         || chunk_is_token(pc, CT_ANGLE_CLOSE)
         || chunk_is_token(pc, CT_SQUARE_CLOSE)
         || chunk_is_token(pc, CT_TSQUARE)
         || chunk_is_token(pc, CT_FPAREN_OPEN)
         || chunk_is_token(pc, CT_QUESTION)
         || (  chunk_is_token(pc, CT_COLON)
            && !we_have_a_case_before)
         || (  chunk_is_token(pc, CT_BRACE_OPEN)
            && (  get_chunk_parent_type(pc) == CT_NONE
               || get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)))
      {
         log_pcf_flags(LFCNR, pc->flags);
         auto brace_open = chunk_get_next_ncnl(pc);

         if (  chunk_is_token(brace_open, CT_BRACE_OPEN)
            && (  get_chunk_parent_type(brace_open) == CT_NONE
               || get_chunk_parent_type(brace_open) == CT_ASSIGN
               || get_chunk_parent_type(brace_open) == CT_RETURN
               || get_chunk_parent_type(brace_open) == CT_BRACED_INIT_LIST))
         {
            log_pcf_flags(LFCNR, brace_open->flags);
            auto brace_close = chunk_skip_to_match(next);

            if (chunk_is_token(brace_close, CT_BRACE_CLOSE))
            {
               set_chunk_parent(brace_open, CT_BRACED_INIT_LIST);
               set_chunk_parent(brace_close, CT_BRACED_INIT_LIST);

               tmp = chunk_get_next_ncnl(brace_close);

               if (tmp)
               {
                  chunk_flags_clr(tmp, PCF_EXPR_START | PCF_STMT_START);
               }
               // TODO: Change pc->type CT_WORD -> CT_TYPE
               // for the case CT_ASSIGN (and others).

               // TODO: Move this block to the fix_fcn_call_args function.
               if (chunk_is_token(pc, CT_WORD) && pc->flags.test(PCF_IN_FCN_CALL))
               {
                  set_chunk_type(pc, CT_TYPE);
               }
            }
         }
      }
   }

   // Check for stuff that can only occur at the start of an expression
   if (  pc->flags.test(PCF_EXPR_START)
      || (prev->flags.test(PCF_EXPR_START) && get_chunk_parent_type(pc) == CT_OC_AT))
   {
      // Change STAR, MINUS, and PLUS in the easy cases
      if (chunk_is_token(pc, CT_STAR))
      {
         // issue #596
         // [0x100062020:IN_SPAREN,IN_FOR,STMT_START,EXPR_START,PUNCTUATOR]
         // prev->type is CT_COLON ==> CT_DEREF
         if (chunk_is_token(prev, CT_ANGLE_CLOSE))
         {
            set_chunk_type(pc, CT_PTR_TYPE);
         }
         else if (chunk_is_token(prev, CT_COLON))
         {
            set_chunk_type(pc, CT_DEREF);
         }
         else
         {
            set_chunk_type(pc, CT_DEREF);
         }
      }

      if (  language_is_set(LANG_CPP)
         && chunk_is_token(pc, CT_CARET)
         && chunk_is_token(prev, CT_ANGLE_CLOSE))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }

      if (  language_is_set(LANG_CS)
         && (chunk_is_token(pc, CT_QUESTION))
         && (chunk_is_token(prev, CT_ANGLE_CLOSE)))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }

      if (chunk_is_token(pc, CT_MINUS))
      {
         set_chunk_type(pc, CT_NEG);
      }

      if (chunk_is_token(pc, CT_PLUS))
      {
         set_chunk_type(pc, CT_POS);
      }

      if (chunk_is_token(pc, CT_INCDEC_AFTER))
      {
         set_chunk_type(pc, CT_INCDEC_BEFORE);
      }

      if (chunk_is_token(pc, CT_AMP))
      {
         if (chunk_is_token(prev, CT_ANGLE_CLOSE))             // Issue #2324
         {
            set_chunk_type(pc, CT_BYREF);
         }
         else
         {
            set_chunk_type(pc, CT_ADDR);
         }
      }

      if (chunk_is_token(pc, CT_CARET))
      {
         if (language_is_set(LANG_C | LANG_CPP | LANG_OC))
         {
            // This is likely the start of a block literal
            handle_oc_block_literal(pc);
         }
      }
   }

   // Detect a variable definition that starts with struct/enum/union/class
   if (  !pc->flags.test(PCF_IN_TYPEDEF)
      && get_chunk_parent_type(prev) != CT_CPP_CAST
      && !prev->flags.test(PCF_IN_FCN_DEF)
      && (  chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_UNION)
         || chunk_is_token(pc, CT_CLASS)
         || chunk_is_token(pc, CT_ENUM)))
   {
      tmp = chunk_skip_dc_member(next);

      if ((chunk_is_token(tmp, CT_TYPE) || chunk_is_token(tmp, CT_WORD)))
      {
         set_chunk_parent(tmp, pc->type);
         set_chunk_type(tmp, CT_TYPE);

         tmp = chunk_get_next_ncnl(tmp);
      }

      if (chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         tmp = chunk_skip_to_match(tmp);

         if (tmp != nullptr)
         {
            tmp = chunk_get_next_ncnl(tmp);
         }
      }

      if (  tmp != nullptr
         && (chunk_is_ptr_operator(tmp) || chunk_is_token(tmp, CT_WORD)))
      {
         mark_variable_definition(tmp);
      }
   }

   /*
    * Change the parenthesis pair after a function/macro-function
    * CT_PAREN_OPEN => CT_FPAREN_OPEN
    */
   if (chunk_is_token(pc, CT_MACRO_FUNC))
   {
      flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_MACRO_FUNC, false);
   }

   if (  chunk_is_token(pc, CT_MACRO_OPEN)
      || chunk_is_token(pc, CT_MACRO_ELSE)
      || chunk_is_token(pc, CT_MACRO_CLOSE))
   {
      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, pc->type, false);
      }
   }

   if (chunk_is_token(pc, CT_DELETE) && chunk_is_token(next, CT_TSQUARE))
   {
      set_chunk_parent(next, CT_DELETE);
   }

   // Change CT_STAR to CT_PTR_TYPE or CT_ARITH or CT_DEREF
   if (  chunk_is_token(pc, CT_STAR)
      || (language_is_set(LANG_CPP) && chunk_is_token(pc, CT_CARET)))
   {
      if (chunk_is_paren_close(next) || chunk_is_token(next, CT_COMMA))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (language_is_set(LANG_OC) && chunk_is_token(next, CT_STAR))
      {
         /*
          * Change pointer-to-pointer types in OC_MSG_DECLs
          * from ARITH <===> DEREF to PTR_TYPE <===> PTR_TYPE
          */
         set_chunk_type(pc, CT_PTR_TYPE);
         set_chunk_parent(pc, get_chunk_parent_type(prev));

         set_chunk_type(next, CT_PTR_TYPE);
         set_chunk_parent(next, get_chunk_parent_type(pc));
      }
      else if (  chunk_is_token(pc, CT_STAR)
              && (  chunk_is_token(prev, CT_DECLTYPE)
                 || chunk_is_token(prev, CT_SIZEOF)
                 || chunk_is_token(prev, CT_DELETE)
                 || (pc && get_chunk_parent_type(pc) == CT_SIZEOF)))
      {
         set_chunk_type(pc, CT_DEREF);
      }
      else if (  (  chunk_is_token(prev, CT_WORD)
                 && chunk_ends_type(prev)
                 && !prev->flags.test(PCF_IN_FCN_CTOR))
              || chunk_is_token(prev, CT_DC_MEMBER)
              || chunk_is_token(prev, CT_PTR_TYPE))
      {
         LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n   ",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         log_pcf_flags(LFCNR, pc->flags);
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_token(next, CT_SQUARE_OPEN) && !language_is_set(LANG_OC))  // issue # 408
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_token(pc, CT_STAR))
      {
         // Add check for CT_DC_MEMBER CT_WORD CT_STAR sequence
         // to convert CT_WORD into CT_TYPE
         // and CT_STAR into CT_PTR_TYPE
         // look for an assign backward to distinguish between
         //    double result = Constants::PI * factor;
         // and
         //    ::some::name * foo;
         if (  chunk_is_token(prev, CT_WORD)
            && chunk_is_token(prev->prev, CT_DC_MEMBER)
            && language_is_set(LANG_CPP))
         {
            // Issue 1402
            bool assign_found = false;
            tmp = pc;

            while (tmp != nullptr)
            {
               if (chunk_is_token(tmp, CT_SEMICOLON))
               {
                  break;
               }
               else if (chunk_is_token(tmp, CT_ASSIGN))
               {
                  assign_found = true;
                  break;
               }
               tmp = chunk_get_prev_ncnlni(tmp); // Issue #2279
            }

            if (assign_found)
            {
               // double result = Constants::PI * factor;
               set_chunk_type(pc, CT_ARITH);
            }
            else
            {
               //    ::some::name * foo;
               set_chunk_type(prev, CT_TYPE);
               set_chunk_type(pc, CT_PTR_TYPE);
            }
         }

         /*
          * A star can have three meanings
          * 1. CT_DEREF    = pointer dereferencing
          * 2. CT_PTR_TYPE = pointer definition
          * 3. CT_ARITH    = arithmetic multiplication
          *
          * most PCF_PUNCTUATOR chunks except a paren close would make this
          * a deref. A paren close may end a cast or may be part of a macro fcn.
          */
         if (chunk_is_token(prev, CT_TYPE))
         {
            set_chunk_type(pc, CT_PTR_TYPE);
         }
         else if (  chunk_is_token(pc->next, CT_SEMICOLON)      // Issue #2319
                 || (  chunk_is_token(pc->next, CT_STAR)
                    && chunk_is_token(pc->next->next, CT_SEMICOLON)))
         {
            // example:
            //    using AbstractLinkPtr = AbstractLink*;
            //    using AbstractLinkPtrPtr = AbstractLink**;
            set_chunk_type(pc, CT_PTR_TYPE);
         }
         else if (  (  get_chunk_parent_type(pc) == CT_FUNC_DEF
                    && (chunk_is_opening_brace(next) || chunk_is_star(pc->next)))
                 || (next->type == CT_QUALIFIER))               // Issue #2648
         {
            // example:
            // auto getComponent(Color *color) -> Component * {
            // auto getComponent(Color *color) -> Component ** {
            // auto getComponent(Color *color) -> Component * _Nonnull
            set_chunk_type(pc, CT_PTR_TYPE);
         }
         else if (  chunk_is_token(pc->next, CT_SEMICOLON)      // Issue #2319
                 || (  chunk_is_token(pc->next, CT_STAR)
                    && chunk_is_token(pc->next->next, CT_STAR)))
         {
            // more pointers are NOT yet possible
            fprintf(stderr, "Too many pointers\n");
            fprintf(stderr, "at line %zu, column %zu.\n", pc->orig_line, pc->orig_col);
            fprintf(stderr, "Please make a report.\n");
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         else
         {
            // Issue 1402
            set_chunk_type(pc,
                           (  prev->flags.test(PCF_PUNCTUATOR)
                           && (  !chunk_is_paren_close(prev)
                              || chunk_is_token(prev, CT_SPAREN_CLOSE)
                              || get_chunk_parent_type(prev) == CT_MACRO_FUNC)
                           && prev->type != CT_SQUARE_CLOSE
                           && prev->type != CT_DC_MEMBER) ? CT_DEREF : CT_ARITH);
         }

         if (pc->flags.test(PCF_IN_TYPEDEF))  // Issue #1255/#633
         {
            tmp = pc;

            while (tmp != nullptr)
            {
               if (  chunk_is_token(tmp, CT_SEMICOLON)
                  || chunk_is_token(tmp, CT_BRACE_OPEN))
               {
                  break;
               }
               else if (chunk_is_token(tmp, CT_TYPEDEF))
               {
                  set_chunk_type(pc, CT_PTR_TYPE);
               }
               tmp = chunk_get_prev_ncnlni(tmp); // Issue #2279
            }
         }
      }
   }

   if (chunk_is_token(pc, CT_AMP))
   {
      if (chunk_is_token(prev, CT_DELETE))
      {
         set_chunk_type(pc, CT_ADDR);
      }
      else if (chunk_is_token(prev, CT_TYPE))
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else if (chunk_is_token(next, CT_FPAREN_CLOSE) || chunk_is_token(next, CT_COMMA))
      {
         // fix the bug #654
         // connect(&mapper, SIGNAL(mapped(QString &)), this, SLOT(onSomeEvent(QString &)));
         set_chunk_type(pc, CT_BYREF);
      }
      else if (get_chunk_parent_type(pc) == CT_USING_ALIAS)
      {
         // fix the Issue # 1689
         // using reference = value_type &;
         set_chunk_type(pc->prev, CT_TYPE);
         set_chunk_type(pc, CT_BYREF);
      }
      else
      {
         // Issue # 1398
         if (  pc->flags.test(PCF_IN_FCN_DEF)
            && chunk_is_token(prev, CT_WORD)
            && chunk_is_token(pc, CT_AMP)
            && chunk_is_token(next, CT_WORD))
         {
            /*
             * Change CT_WORD before CT_AMP before CT_WORD to CT_TYPE
             */
            set_chunk_type(prev, CT_TYPE);
         }
         else
         {
            set_chunk_type(pc, CT_ARITH);

            if (chunk_is_token(prev, CT_WORD))
            {
               tmp = chunk_get_prev_ncnlni(prev); // Issue #2279

               if (tmp != nullptr)
               {
                  if (  chunk_is_semicolon(tmp)
                     || chunk_is_token(tmp, CT_BRACE_OPEN)
                     || chunk_is_token(tmp, CT_QUALIFIER))
                  {
                     set_chunk_type(pc, CT_BYREF);
                     set_chunk_type(prev, CT_TYPE);

                     if (!(  chunk_is_token(next, CT_OPERATOR)
                          || chunk_is_token(next, CT_TYPE)
                          || chunk_is_token(next, CT_DC_MEMBER)))
                     {
                        LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', set PCF_VAR_1ST\n",
                                __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
                        chunk_flags_set(next, PCF_VAR_1ST);
                     }
                  }
                  else if (chunk_is_token(tmp, CT_DC_MEMBER))
                  {
                     set_chunk_type(prev, CT_TYPE);

                     if (!chunk_is_token(next, CT_TYPE))            // Issue #2103
                     {
                        set_chunk_type(pc, CT_BYREF);
                     }
                  }
               }
            }
         }
      }
   }

   if (chunk_is_token(pc, CT_MINUS) || chunk_is_token(pc, CT_PLUS))
   {
      if (  chunk_is_token(prev, CT_POS)
         || chunk_is_token(prev, CT_NEG)
         || chunk_is_token(prev, CT_ARITH))
      {
         set_chunk_type(pc, (pc->type == CT_MINUS) ? CT_NEG : CT_POS);
      }
      else if (chunk_is_token(prev, CT_OC_CLASS))
      {
         set_chunk_type(pc, (chunk_is_token(pc, CT_MINUS)) ? CT_NEG : CT_POS);
      }
      else
      {
         set_chunk_type(pc, CT_ARITH);
      }
   }

   /*
    * Bug # 634
    * Check for extern "C" NSString* i;
    * NSString is a type
    * change CT_WORD => CT_TYPE     for pc
    * change CT_STAR => CT_PTR_TYPE for pc-next
    */
   if (chunk_is_token(pc, CT_WORD))     // here NSString
   {
      if (pc->next != nullptr)          // here *
      {
         if (pc->next->type == CT_STAR) // here *
         {
            // compare text with "C" to find extern "C" instructions
            if (pc->prev != nullptr)
            {
               if (pc->prev->type == CT_STRING)
               {
                  if (unc_text::compare(pc->prev->text(), "\"C\"") == 0)
                  {
                     if (pc->prev->prev->type == CT_EXTERN)
                     {
                        set_chunk_type(pc, CT_TYPE);            // change CT_WORD => CT_TYPE
                        set_chunk_type(pc->next, CT_PTR_TYPE);  // change CT_STAR => CT_PTR_TYPE
                     }
                  }
               }
            }

            // Issue #322 STDMETHOD(GetValues)(BSTR bsName, REFDATA** pData);
            if (  (pc->next->next != nullptr)
               && pc->next->next->type == CT_STAR
               && pc->flags.test(PCF_IN_CONST_ARGS))
            {
               // change CT_STAR => CT_PTR_TYPE
               set_chunk_type(pc->next, CT_PTR_TYPE);
               set_chunk_type(pc->next->next, CT_PTR_TYPE);
            }

            // Issue #222 whatever3 *(func_ptr)( whatever4 *foo2, ...
            if (  (pc->next->next != nullptr)
               && pc->next->next->type == CT_WORD
               && pc->flags.test(PCF_IN_FCN_DEF))
            {
               // look for the opening parenthesis
               // Issue 1403
               tmp = chunk_get_prev_type(pc, CT_FPAREN_OPEN, pc->level - 1);

               if (  tmp != nullptr
                  && get_chunk_parent_type(tmp) != CT_FUNC_CTOR_VAR)
               {
                  set_chunk_type(pc->next, CT_PTR_TYPE);
               }
            }
         }
      }
   }

   /*
    * Bug # 634
    * Check for __attribute__((visibility ("default"))) NSString* i;
    * NSString is a type
    * change CT_WORD => CT_TYPE     for pc
    * change CT_STAR => CT_PTR_TYPE for pc-next
    */
   if (chunk_is_token(pc, CT_WORD))     // here NSString
   {
      if (pc->next != nullptr)          // here *
      {
         if (pc->next->type == CT_STAR) // here *
         {
            tmp = pc;

            while ((tmp != nullptr))
            {
               if (chunk_is_token(tmp, CT_ATTRIBUTE))
               {
                  LOG_FMT(LFCNR, "%s(%d): ATTRIBUTE found, type is %s, text() '%s'\n",
                          __func__, __LINE__, get_token_name(tmp->type), tmp->text());
                  LOG_FMT(LFCNR, "for token, type is %s, text() '%s'\n", get_token_name(pc->type), pc->text());
                  // change CT_WORD => CT_TYPE
                  set_chunk_type(pc, CT_TYPE);
                  // change CT_STAR => CT_PTR_TYPE
                  set_chunk_type(pc->next, CT_PTR_TYPE);
               }

               if (tmp->flags.test(PCF_STMT_START))
               {
                  // we are at beginning of the line
                  break;
               }
               tmp = chunk_get_prev(tmp);
            }
         }
      }
   }

   /*
    * Issue # 1689
    * Check for using reference = value_type&;
    * is it a Type alias, alias template?
    */
   if (chunk_is_token(pc, CT_USING))
   {
      // look for CT_ASSIGN before CT_SEMICOLON at the end of the statement
      bool    assign_found = false;
      bool    is_preproc   = pc->flags.test(PCF_IN_PREPROC);
      chunk_t *temp;

      for (temp = pc; temp != nullptr; temp = chunk_get_next_ncnl(temp))
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s\n",
                 __func__, __LINE__, temp->orig_line, temp->orig_col, temp->text(), get_token_name(temp->type));

         if (chunk_is_token(temp, CT_ASSIGN))
         {
            assign_found = true;
            break;
         }

         if (  chunk_is_token(temp, CT_SEMICOLON)
            || (  is_preproc
               && (  !temp->flags.test(PCF_IN_PREPROC)
                  || chunk_is_token(temp, CT_PREPROC))))
         {
            break;
         }
      }

      if (assign_found)
      {
         // it is a Type alias, alias template
         for (temp = pc; temp != nullptr; temp = chunk_get_next_ncnl(temp))
         {
            if (get_chunk_parent_type(temp) == CT_NONE)
            {
               set_chunk_parent(temp, CT_USING_ALIAS);
            }

            if (  chunk_is_token(temp, CT_SEMICOLON)
               || (  is_preproc
                  && (  !temp->flags.test(PCF_IN_PREPROC)
                     || chunk_is_token(temp, CT_PREPROC))))
            {
               break;
            }
         }
      }
   }

   // Issue #548: inline T && someFunc(foo * *p, bar && q) { }
   if (  pc->type == CT_BOOL
      && !pc->flags.test(PCF_IN_PREPROC)
      && chunk_is_str(pc, "&&", 2)
      && chunk_ends_type(pc->prev))
   {
      set_chunk_type(pc, CT_BYREF);
   }

   // Issue #1704
   if (  chunk_is_token(pc, CT_INCDEC_AFTER)
      && pc->flags.test(PCF_IN_PREPROC))
   {
      chunk_t *tmp_2 = chunk_get_next(pc);
      log_pcf_flags(LFTYPE, pc->flags);

      if (chunk_is_token(tmp_2, CT_WORD))
      {
         set_chunk_type(pc, CT_INCDEC_BEFORE);
      }
   }
} // do_symbol_check


static void check_double_brace_init(chunk_t *bo1)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LJDBI, "%s(%d): orig_line is %zu, orig_col is %zu", __func__, __LINE__, bo1->orig_line, bo1->orig_col);
   chunk_t *pc = chunk_get_prev_ncnlni(bo1);   // Issue #2279

   if (pc == nullptr)
   {
      return;
   }

   if (chunk_is_paren_close(pc))
   {
      chunk_t *bo2 = chunk_get_next(bo1);

      if (bo2 == nullptr)
      {
         return;
      }

      if (chunk_is_token(bo2, CT_BRACE_OPEN))
      {
         // found a potential double brace
         chunk_t *bc2 = chunk_skip_to_match(bo2);

         if (bc2 == nullptr)
         {
            return;
         }
         chunk_t *bc1 = chunk_get_next(bc2);

         if (bc1 == nullptr)
         {
            return;
         }

         if (chunk_is_token(bc1, CT_BRACE_CLOSE))
         {
            LOG_FMT(LJDBI, " - end, orig_line is %zu, orig_col is %zu\n", bc2->orig_line, bc2->orig_col);
            // delete bo2 and bc1
            bo1->str         += bo2->str;
            bo1->orig_col_end = bo2->orig_col_end;
            chunk_del(bo2);
            set_chunk_parent(bo1, CT_DOUBLE_BRACE);

            bc2->str         += bc1->str;
            bc2->orig_col_end = bc1->orig_col_end;
            chunk_del(bc1);
            set_chunk_parent(bc2, CT_DOUBLE_BRACE);
            return;
         }
      }
   }
   LOG_FMT(LJDBI, " - no\n");
} // check_double_brace_init


void fix_symbols(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t dummy;

   cpd.unc_stage = unc_stage_e::FIX_SYMBOLS;

   mark_define_expressions();

   bool is_cpp  = language_is_set(LANG_CPP);
   bool is_java = language_is_set(LANG_JAVA);

   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (  chunk_is_token(pc, CT_FUNC_WRAP)
         || chunk_is_token(pc, CT_TYPE_WRAP))
      {
         handle_wrap(pc);
      }

      if (chunk_is_token(pc, CT_ASSIGN))
      {
         mark_lvalue(pc);
      }
      // a brace immediately preceeded by word in C++11 is an initializer list though it may also
      // by a type casting initializer list if the word is really a type; sadly unucustify knows
      // only builtin types and knows nothing of user-defined types
      chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279

      if (  is_cpp
         && chunk_is_token(pc, CT_BRACE_OPEN)
         && (  chunk_is_token(prev, CT_WORD)
            || chunk_is_token(prev, CT_TYPE)))
      {
         mark_lvalue(pc);
      }

      if (  is_java
         && chunk_is_token(pc, CT_BRACE_OPEN))
      {
         check_double_brace_init(pc);
      }

      if (chunk_is_token(pc, CT_ATTRIBUTE))
      {
         chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

         if (  next != nullptr
            && chunk_is_token(next, CT_PAREN_OPEN))
         {
            flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, CT_ATTRIBUTE, false);
         }
      }
   }

   pc = chunk_get_head();

   if (pc == nullptr)
   {
      return;
   }

   if (  chunk_is_newline(pc)
      || chunk_is_comment(pc))
   {
      pc = chunk_get_next_ncnl(pc);
   }

   while (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_IGNORED))
      {
         pc = chunk_get_next_ncnl(pc);
         continue;
      }
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line       is %zu, orig_col is %zu, text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      chunk_t *prev = chunk_get_prev_ncnlni(pc, scope_e::PREPROC);   // Issue #2279

      if (prev == nullptr)
      {
         prev = &dummy;
      }
      else
      {
         // Issue #2279
         LOG_FMT(LFCNR, "%s(%d): prev(ni)->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text(), get_token_name(prev->type));
      }
      chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

      if (next == nullptr)
      {
         next = &dummy;
      }
      else
      {
         // Issue #2279
         LOG_FMT(LFCNR, "%s(%d): next->orig_line     is %zu, orig_col is %zu, text() is '%s', type is %s\n",
                 __func__, __LINE__, next->orig_line, next->orig_col, next->text(), get_token_name(next->type));
      }
      LOG_FMT(LFCNR, "%s(%d): do_symbol_check(%s, %s, %s)\n",
              __func__, __LINE__, prev->text(), pc->text(), next->text());
      do_symbol_check(prev, pc, next);
      pc = chunk_get_next_ncnl(pc);
   }
   pawn_add_virtual_semicolons();
   process_returns();

   /*
    * 2nd pass - handle variable definitions
    * REVISIT: We need function params marked to do this (?)
    */
   pc = chunk_get_head();
   int square_level = -1;

   while (pc != nullptr)
   {
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type), get_token_name(pc->parent_type));

      // Can't have a variable definition inside [ ]
      if (square_level < 0)
      {
         if (chunk_is_token(pc, CT_SQUARE_OPEN))
         {
            square_level = pc->level;
         }
      }
      else
      {
         if (pc->level <= static_cast<size_t>(square_level))
         {
            square_level = -1;
         }
      }

      if (  chunk_is_token(pc, CT_EXTERN)
         && language_is_set(LANG_ALLC))
      {
         chunk_t *next = chunk_get_next_ncnl(pc);

         if (chunk_is_token(next, CT_STRING))
         {
            chunk_t *tmp = chunk_get_next_ncnl(next);

            while (tmp != nullptr)
            {
               if (  (chunk_is_token(tmp, CT_TYPE))
                  || (chunk_is_token(tmp, CT_BRACE_OPEN))
                  || (chunk_is_token(tmp, CT_ATTRIBUTE)))
               {
                  break;
               }

               if (chunk_is_token(tmp, CT_WORD))
               {
                  chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
                  break;
               }
               tmp = chunk_get_next_ncnl(tmp);
            }
         }
      }

      if (  chunk_is_token(pc, CT_ATTRIBUTE)
         && language_is_set(LANG_ALLC))
      {
         chunk_t *tmp = skip_attribute_next(pc);

         if (chunk_is_token(tmp, CT_WORD))
         {
            chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
         }
      }

      if (  chunk_is_token(pc, CT_BRACE_OPEN)                       // Issue #2332
         && get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)
      {
         LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', look for CT_BRACE_OPEN\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         pc = chunk_get_next_type(pc, CT_BRACE_CLOSE, pc->level);
      }
      /*
       * A variable definition is possible after at the start of a statement
       * that starts with: DC_MEMBER, QUALIFIER, TYPE, or WORD
       */
      // Issue #2279
      // Issue #2478
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n   ",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type), get_token_name(pc->parent_type));
      log_pcf_flags(LFCNR, pc->flags);

      if (  (square_level < 0)
         && pc->flags.test(PCF_STMT_START)
         && (  chunk_is_token(pc, CT_QUALIFIER)
            || chunk_is_token(pc, CT_TYPE)
            || chunk_is_token(pc, CT_TYPENAME)
            || chunk_is_token(pc, CT_DC_MEMBER)                         // Issue #2478
            || chunk_is_token(pc, CT_WORD))
         && get_chunk_parent_type(pc) != CT_ENUM
         && !pc->flags.test(PCF_IN_ENUM))
      {
         pc = fix_variable_definition(pc);
      }
      else
      {
         pc = chunk_get_next_ncnl(pc);
      }
   }
} // fix_symbols


static void process_returns(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   pc = chunk_get_head();

   while (pc != nullptr)
   {
      if (pc->type != CT_RETURN)
      {
         pc = chunk_get_next_type(pc, CT_RETURN, -1);
         continue;
      }
      pc = process_return(pc);
   }
}


static chunk_t *process_return(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *temp;
   chunk_t *semi;
   chunk_t *cpar;
   chunk_t chunk;

   // grab next and bail if it is a semicolon
   next = chunk_ppa_get_next_ncnl(pc);

   if (  next == nullptr || chunk_is_semicolon(next)
      || chunk_is_token(next, CT_NEWLINE))
   {
      return(next);
   }
   log_rule_B("nl_return_expr");

   if (  options::nl_return_expr() != IARF_IGNORE
      && !pc->flags.test(PCF_IN_PREPROC))
   {
      newline_iarf(pc, options::nl_return_expr());
   }

   if (chunk_is_token(next, CT_PAREN_OPEN))
   {
      // See if the return is fully paren'd
      cpar = chunk_get_next_type(next, CT_PAREN_CLOSE, next->level);

      if (cpar == nullptr)
      {
         return(nullptr);
      }
      semi = chunk_ppa_get_next_ncnl(cpar);

      if (semi == nullptr)
      {
         return(nullptr);
      }

      if (chunk_is_token(semi, CT_NEWLINE) || chunk_is_semicolon(semi))
      {
         log_rule_B("mod_paren_on_return");

         if (options::mod_paren_on_return() == IARF_REMOVE)
         {
            LOG_FMT(LRETURN, "%s(%d): removing parens on orig_line %zu\n",
                    __func__, __LINE__, pc->orig_line);

            // lower the level of everything
            for (temp = next; temp != cpar; temp = chunk_get_next(temp))
            {
               if (temp->level == 0)
               {
                  fprintf(stderr, "%s(%d): temp->level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                          __func__, __LINE__, temp->orig_line, temp->orig_col);
                  log_flush(true);
                  exit(EX_SOFTWARE);
               }
               temp->level--;
            }

            // delete the parenthesis
            chunk_del(next);
            chunk_del(cpar);

            // back up following chunks
            temp = semi;

            while (temp != nullptr && temp->type != CT_NEWLINE)
            {
               temp->column       = temp->column - 2;
               temp->orig_col     = temp->orig_col - 2;
               temp->orig_col_end = temp->orig_col_end - 2;
               temp               = chunk_get_next(temp);
            }
         }
         else
         {
            LOG_FMT(LRETURN, "%s(%d): keeping parens on orig_line %zu\n",
                    __func__, __LINE__, pc->orig_line);

            // mark & keep them
            set_chunk_parent(next, CT_RETURN);
            set_chunk_parent(cpar, CT_RETURN);
         }
         return(semi);
      }
   }
   // We don't have a fully paren'd return. Should we add some?
   log_rule_B("mod_paren_on_return");

   if (!(options::mod_paren_on_return() & IARF_ADD))
   {
      return(next);
   }

   // Issue #1917
   // Never add parens to a braced init list; that breaks the code
   //   return {args...};    // C++11 type elision; okay
   //   return ({args...});  // ill-formed
   if (  language_is_set(LANG_CPP) && chunk_is_token(next, CT_BRACE_OPEN)
      && get_chunk_parent_type(next) == CT_BRACED_INIT_LIST)
   {
      LOG_FMT(LRETURN, "%s(%d): not adding parens around braced initializer"
              " on orig_line %zd\n",
              __func__, __LINE__, pc->orig_line);
      return(next);
   }
   // find the next semicolon on the same level
   semi = next;

   if (pc->flags.test(PCF_IN_PREPROC))
   {
      while ((semi = semi->next) != nullptr)
      {
         if (!semi->flags.test(PCF_IN_PREPROC))
         {
            break;
         }

         if (semi->level < pc->level)
         {
            return(semi);
         }

         if (chunk_is_semicolon(semi) && pc->level == semi->level)
         {
            break;
         }
      }
   }
   else
   {
      while ((semi = chunk_get_next(semi)) != nullptr)
      {
         if (semi->level < pc->level)
         {
            return(semi);
         }

         if (chunk_is_semicolon(semi) && pc->level == semi->level)
         {
            break;
         }
      }
   }

   if (semi)
   {
      // add the parenthesis
      set_chunk_type(&chunk, CT_PAREN_OPEN);
      set_chunk_parent(&chunk, CT_RETURN);
      chunk.str         = "(";
      chunk.level       = pc->level;
      chunk.brace_level = pc->brace_level;
      chunk.orig_line   = pc->orig_line;
      chunk.orig_col    = next->orig_col - 1;
      chunk.flags       = pc->flags & PCF_COPY_FLAGS;
      chunk_add_before(&chunk, next);

      set_chunk_type(&chunk, CT_PAREN_CLOSE);
      chunk.str       = ")";
      chunk.orig_line = semi->orig_line;
      chunk.orig_col  = semi->orig_col - 1;
      cpar            = chunk_add_before(&chunk, semi);

      LOG_FMT(LRETURN, "%s(%d): added parens on orig_line %zu\n",
              __func__, __LINE__, pc->orig_line);

      for (temp = next; temp != cpar; temp = chunk_get_next(temp))
      {
         temp->level++;
      }
   }
   return(semi);
} // process_return


static bool is_oc_block(chunk_t *pc)
{
   return(  pc != nullptr
         && (  get_chunk_parent_type(pc) == CT_OC_BLOCK_TYPE
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_ARG
            || get_chunk_parent_type(pc) == CT_OC_BLOCK
            || chunk_is_token(pc, CT_OC_BLOCK_CARET)
            || (pc->next != nullptr && pc->next->type == CT_OC_BLOCK_CARET)
            || (pc->prev != nullptr && pc->prev->type == CT_OC_BLOCK_CARET)));
}


void mark_comments(void)
{
   LOG_FUNC_ENTRY();

   cpd.unc_stage = unc_stage_e::MARK_COMMENTS;

   bool    prev_nl = true;
   chunk_t *cur    = chunk_get_head();

   while (cur != nullptr)
   {
      chunk_t *next   = chunk_get_next_nvb(cur);
      bool    next_nl = (next == nullptr) || chunk_is_newline(next);

      if (chunk_is_comment(cur))
      {
         if (next_nl && prev_nl)
         {
            set_chunk_parent(cur, CT_COMMENT_WHOLE);
         }
         else if (next_nl)
         {
            set_chunk_parent(cur, CT_COMMENT_END);
         }
         else if (prev_nl)
         {
            set_chunk_parent(cur, CT_COMMENT_START);
         }
         else
         {
            set_chunk_parent(cur, CT_COMMENT_EMBED);
         }
      }
      prev_nl = chunk_is_newline(cur);
      cur     = next;
   }
}


static void handle_cpp_template(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   chunk_t *tmp = chunk_get_next_ncnl(pc);

   if (tmp->type != CT_ANGLE_OPEN)
   {
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);

   size_t level = tmp->level;

   while ((tmp = chunk_get_next(tmp)) != nullptr)
   {
      if (chunk_is_token(tmp, CT_CLASS) || chunk_is_token(tmp, CT_STRUCT))
      {
         set_chunk_type(tmp, CT_TYPE);
      }
      else if (chunk_is_token(tmp, CT_ANGLE_CLOSE) && tmp->level == level)
      {
         set_chunk_parent(tmp, CT_TEMPLATE);
         break;
      }
   }

   if (tmp != nullptr)
   {
      tmp = chunk_get_next_ncnl(tmp);

      if (chunk_is_token(tmp, CT_CLASS) || chunk_is_token(tmp, CT_STRUCT))
      {
         set_chunk_parent(tmp, CT_TEMPLATE);

         // REVISIT: This may be a bit risky - might need to track the { };
         tmp = chunk_get_next_type(tmp, CT_SEMICOLON, tmp->level);

         if (tmp != nullptr)
         {
            set_chunk_parent(tmp, CT_TEMPLATE);
         }
      }
   }
} // handle_cpp_template


static void handle_cpp_lambda(chunk_t *sq_o)
{
   LOG_FUNC_ENTRY();

   chunk_t *ret = nullptr;

   // abort if type of the previous token is not contained in this whitelist
   chunk_t *prev = chunk_get_prev_ncnlni(sq_o);   // Issue #2279

   if (  prev == nullptr
      || (  prev->type != CT_ASSIGN
         && prev->type != CT_COMMA
         && prev->type != CT_PAREN_OPEN   // allow Js like self invoking lambda syntax: ([](){})();
         && prev->type != CT_FPAREN_OPEN
         && prev->type != CT_SQUARE_OPEN
         && prev->type != CT_BRACE_OPEN
         && prev->type != CT_SEMICOLON
         && prev->type != CT_RETURN))
   {
      return;
   }
   chunk_t *sq_c = sq_o; // assuming '[]'

   if (chunk_is_token(sq_o, CT_SQUARE_OPEN))
   {
      // make sure there is a ']'
      sq_c = chunk_skip_to_match(sq_o);

      if (!sq_c)
      {
         return;
      }
   }
   chunk_t *pa_o = chunk_get_next_ncnl(sq_c);

   // check to see if there is a lambda-specifier in the pa_o chunk;
   // assuming chunk is CT_EXECUTION_CONTEXT, ignore lambda-specifier
   while (pa_o->type == CT_EXECUTION_CONTEXT)
   {
      // set pa_o to next chunk after this specifier
      pa_o = chunk_get_next_ncnl(pa_o);
   }

   if (pa_o == nullptr)
   {
      return;
   }
   chunk_t *pa_c = nullptr;

   // lambda-declarator '( params )' is optional
   if (chunk_is_token(pa_o, CT_PAREN_OPEN))
   {
      // and now find the ')'
      pa_c = chunk_skip_to_match(pa_o);

      if (pa_c == nullptr)
      {
         return;
      }
   }
   // Check for 'mutable' keyword: '[]() mutable {}' or []() mutable -> ret {}
   chunk_t *br_o = pa_c ? chunk_get_next_ncnl(pa_c) : pa_o;

   if (chunk_is_str(br_o, "mutable", 7))
   {
      br_o = chunk_get_next_ncnl(br_o);
   }
   //TODO: also check for exception and attribute between [] ... {}

   // skip possible arrow syntax: '-> ret'
   if (chunk_is_str(br_o, "->", 2))
   {
      ret = br_o;
      // REVISIT: really should check the stuff we are skipping
      br_o = chunk_get_next_type(br_o, CT_BRACE_OPEN, br_o->level);
   }

   if (  br_o == nullptr
      || br_o->type != CT_BRACE_OPEN)
   {
      return;
   }
   // and now find the '}'
   chunk_t *br_c = chunk_skip_to_match(br_o);

   if (br_c == nullptr)
   {
      return;
   }

   // This looks like a lambda expression
   if (chunk_is_token(sq_o, CT_TSQUARE))
   {
      // split into two chunks
      chunk_t nc;

      nc = *sq_o;
      set_chunk_type(sq_o, CT_SQUARE_OPEN);
      sq_o->str.resize(1);
      /*
       * bug # 664
       *
       * The original orig_col of CT_SQUARE_CLOSE is stored at orig_col_end
       * of CT_TSQUARE. CT_SQUARE_CLOSE orig_col and orig_col_end values
       * are calculate from orig_col_end of CT_TSQUARE.
       */
      nc.orig_col        = sq_o->orig_col_end - 1;
      nc.column          = static_cast<int>(nc.orig_col);
      nc.orig_col_end    = sq_o->orig_col_end;
      sq_o->orig_col_end = sq_o->orig_col + 1;

      set_chunk_type(&nc, CT_SQUARE_CLOSE);
      nc.str.pop_front();
      sq_c = chunk_add_after(&nc, sq_o);
   }
   set_chunk_parent(sq_o, CT_CPP_LAMBDA);
   set_chunk_parent(sq_c, CT_CPP_LAMBDA);

   if (pa_c != nullptr)
   {
      set_chunk_type(pa_o, CT_FPAREN_OPEN);
      set_chunk_parent(pa_o, CT_CPP_LAMBDA);
      set_chunk_type(pa_c, CT_FPAREN_CLOSE);
      set_chunk_parent(pa_c, CT_CPP_LAMBDA);
   }
   set_chunk_parent(br_o, CT_CPP_LAMBDA);
   set_chunk_parent(br_c, CT_CPP_LAMBDA);

   if (ret != nullptr)
   {
      set_chunk_type(ret, CT_CPP_LAMBDA_RET);
      ret = chunk_get_next_ncnl(ret);

      while (ret != br_o)
      {
         make_type(ret);
         ret = chunk_get_next_ncnl(ret);
      }
   }

   if (pa_c != nullptr)
   {
      fix_fcn_def_params(pa_o);
   }
   //handle self calling lambda paren
   chunk_t *call_pa_o = chunk_get_next_ncnl(br_c);

   if (chunk_is_token(call_pa_o, CT_PAREN_OPEN))
   {
      chunk_t *call_pa_c = chunk_skip_to_match(call_pa_o);

      if (call_pa_c != nullptr)
      {
         set_chunk_type(call_pa_o, CT_FPAREN_OPEN);
         set_chunk_parent(call_pa_o, CT_FUNC_CALL);
         set_chunk_type(call_pa_c, CT_FPAREN_CLOSE);
         set_chunk_parent(call_pa_c, CT_FUNC_CALL);
      }
   }
} // handle_cpp_lambda


static void handle_d_template(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   chunk_t *name = chunk_get_next_ncnl(pc);
   chunk_t *po   = chunk_get_next_ncnl(name);

   //if (!name || (name->type != CT_WORD && name->type != CT_WORD))  Coverity CID 76000 Same on both sides, 2016-03-16
   if (!name || name->type != CT_WORD)
   {
      // TODO: log an error, expected NAME
      return;
   }

   if (  po == nullptr
      || po->type != CT_PAREN_OPEN)
   {
      // TODO: log an error, expected '('
      return;
   }
   set_chunk_type(name, CT_TYPE);
   set_chunk_parent(name, CT_TEMPLATE);
   set_chunk_parent(po, CT_TEMPLATE);

   ChunkStack cs;
   chunk_t    *tmp = get_d_template_types(cs, po);

   if (  tmp == nullptr
      || tmp->type != CT_PAREN_CLOSE)
   {
      // TODO: log an error, expected ')'
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);

   tmp = chunk_get_next_ncnl(tmp);

   if (tmp->type != CT_BRACE_OPEN)
   {
      // TODO: log an error, expected '{'
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);
   po = tmp;

   tmp = po;

   while (  ((tmp = chunk_get_next_ncnl(tmp)) != nullptr)
         && tmp->level > po->level)
   {
      if (chunk_is_token(tmp, CT_WORD) && chunkstack_match(cs, tmp))
      {
         set_chunk_type(tmp, CT_TYPE);
      }
   }
//   if (!chunk_is_token(tmp, CT_BRACE_CLOSE))
//   {
//      // TODO: log an error, expected '}'
//   }
   set_chunk_parent(tmp, CT_TEMPLATE);
} // handle_d_template


chunk_t *skip_template_next(chunk_t *ang_open)
{
   if (chunk_is_token(ang_open, CT_ANGLE_OPEN))
   {
      chunk_t *pc = chunk_get_next_type(ang_open, CT_ANGLE_CLOSE, ang_open->level);
      return(chunk_get_next_ncnl(pc));
   }
   return(ang_open);
}


static void handle_oc_class(chunk_t *pc)
{
   enum class angle_state_e : unsigned int
   {
      NONE  = 0,
      OPEN  = 1, // '<' found
      CLOSE = 2, // '>' found
   };

   LOG_FUNC_ENTRY();
   chunk_t       *tmp;
   bool          hit_scope     = false;
   bool          passed_name   = false; // Did we pass the name of the class and now there can be only protocols, not generics
   int           generic_level = 0;     // level of depth of generic
   angle_state_e as            = angle_state_e::NONE;

   LOG_FMT(LOCCLASS, "%s(%d): start [%s] [%s] line %zu\n",
           __func__, __LINE__, pc->text(), get_token_name(get_chunk_parent_type(pc)), pc->orig_line);

   if (get_chunk_parent_type(pc) == CT_OC_PROTOCOL)
   {
      tmp = chunk_get_next_ncnl(pc);

      if (chunk_is_semicolon(tmp))
      {
         set_chunk_parent(tmp, get_chunk_parent_type(pc));
         LOG_FMT(LOCCLASS, "%s(%d):   bail on semicolon\n", __func__, __LINE__);
         return;
      }
   }
   tmp = pc;

   while ((tmp = chunk_get_next_nnl(tmp)) != nullptr)
   {
      LOG_FMT(LOCCLASS, "%s(%d):       orig_line is %zu, [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->text());

      if (chunk_is_token(tmp, CT_OC_END))
      {
         break;
      }

      if (chunk_is_token(tmp, CT_PAREN_OPEN))
      {
         passed_name = true;
      }

      if (chunk_is_str(tmp, "<", 1))
      {
         set_chunk_type(tmp, CT_ANGLE_OPEN);

         if (passed_name)
         {
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }
         else
         {
            set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);
            generic_level++;
         }
         as = angle_state_e::OPEN;
      }

      if (chunk_is_str(tmp, ">", 1))
      {
         set_chunk_type(tmp, CT_ANGLE_CLOSE);

         if (passed_name)
         {
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
            as = angle_state_e::CLOSE;
         }
         else
         {
            set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);

            if (generic_level == 0)
            {
               fprintf(stderr, "%s(%d): generic_level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            generic_level--;

            if (generic_level == 0)
            {
               as = angle_state_e::CLOSE;
            }
         }
      }

      if (chunk_is_str(tmp, ">>", 2))
      {
         set_chunk_type(tmp, CT_ANGLE_CLOSE);
         set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);
         split_off_angle_close(tmp);
         generic_level -= 1;

         if (generic_level == 0)
         {
            as = angle_state_e::CLOSE;
         }
      }

      if (  chunk_is_token(tmp, CT_BRACE_OPEN)
         && get_chunk_parent_type(tmp) != CT_ASSIGN)
      {
         as = angle_state_e::CLOSE;
         set_chunk_parent(tmp, CT_OC_CLASS);
         tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);

         if (  tmp != nullptr
            && get_chunk_parent_type(tmp) != CT_ASSIGN)
         {
            set_chunk_parent(tmp, CT_OC_CLASS);
         }
      }
      else if (chunk_is_token(tmp, CT_COLON))
      {
         if (as != angle_state_e::OPEN)
         {
            passed_name = true;
         }
         set_chunk_type(tmp, hit_scope ? CT_OC_COLON : CT_CLASS_COLON);

         if (chunk_is_token(tmp, CT_CLASS_COLON))
         {
            set_chunk_parent(tmp, CT_OC_CLASS);
         }
      }
      else if (chunk_is_str(tmp, "-", 1) || chunk_is_str(tmp, "+", 1))
      {
         as = angle_state_e::CLOSE;

         if (chunk_is_newline(chunk_get_prev(tmp)))
         {
            set_chunk_type(tmp, CT_OC_SCOPE);
            chunk_flags_set(tmp, PCF_STMT_START);
            hit_scope = true;
         }
      }

      if (as == angle_state_e::OPEN)
      {
         if (passed_name)
         {
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }
         else
         {
            set_chunk_parent(tmp, CT_OC_GENERIC_SPEC);
         }
      }
   }

   if (chunk_is_token(tmp, CT_BRACE_OPEN))
   {
      tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);

      if (tmp != nullptr)
      {
         set_chunk_parent(tmp, CT_OC_CLASS);
      }
   }
} // handle_oc_class


static void handle_oc_block_literal(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279
   chunk_t *next = chunk_get_next_ncnl(pc);

   if (  pc == nullptr
      || prev == nullptr
      || next == nullptr)
   {
      return; // let's be paranoid
   }
   /*
    * block literal: '^ RTYPE ( ARGS ) { }'
    * RTYPE and ARGS are optional
    */
   LOG_FMT(LOCBLK, "%s(%d): block literal @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);

   chunk_t *apo = nullptr; // arg paren open
   chunk_t *bbo = nullptr; // block brace open
   chunk_t *bbc;           // block brace close

   LOG_FMT(LOCBLK, "%s(%d):  + scan", __func__, __LINE__);
   chunk_t *tmp;

   for (tmp = next; tmp; tmp = chunk_get_next_ncnl(tmp))
   {
      /* handle '< protocol >' */
      if (chunk_is_str(tmp, "<", 1))
      {
         chunk_t *ao = tmp;
         chunk_t *ac = chunk_get_next_str(ao, ">", 1, ao->level);

         if (ac)
         {
            set_chunk_type(ao, CT_ANGLE_OPEN);
            set_chunk_parent(ao, CT_OC_PROTO_LIST);
            set_chunk_type(ac, CT_ANGLE_CLOSE);
            set_chunk_parent(ac, CT_OC_PROTO_LIST);

            for (tmp = chunk_get_next(ao); tmp != ac; tmp = chunk_get_next(tmp))
            {
               tmp->level += 1;
               set_chunk_parent(tmp, CT_OC_PROTO_LIST);
            }
         }
         tmp = chunk_get_next_ncnl(ac);
      }
      LOG_FMT(LOCBLK, " '%s'", tmp->text());

      if (tmp->level < pc->level || chunk_is_token(tmp, CT_SEMICOLON))
      {
         LOG_FMT(LOCBLK, "[DONE]");
         break;
      }

      if (tmp->level == pc->level)
      {
         if (chunk_is_paren_open(tmp))
         {
            apo = tmp;
            LOG_FMT(LOCBLK, "[PAREN]");
         }

         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            LOG_FMT(LOCBLK, "[BRACE]");
            bbo = tmp;
            break;
         }
      }
   }

   // make sure we have braces
   bbc = chunk_skip_to_match(bbo);

   if (  bbo == nullptr
      || bbc == nullptr)
   {
      LOG_FMT(LOCBLK, " -- no braces found\n");
      return;
   }
   LOG_FMT(LOCBLK, "\n");

   // we are on a block literal for sure
   set_chunk_type(pc, CT_OC_BLOCK_CARET);
   set_chunk_parent(pc, CT_OC_BLOCK_EXPR);

   // handle the optional args
   chunk_t *lbp; // last before paren - end of return type, if any

   if (apo)
   {
      chunk_t *apc = chunk_skip_to_match(apo);  // arg parenthesis close

      if (chunk_is_paren_close(apc))
      {
         LOG_FMT(LOCBLK, " -- marking parens @ apo->orig_line is %zu, apo->orig_col is %zu and apc->orig_line is %zu, apc->orig_col is %zu\n",
                 apo->orig_line, apo->orig_col, apc->orig_line, apc->orig_col);
         flag_parens(apo, PCF_OC_ATYPE, CT_FPAREN_OPEN, CT_OC_BLOCK_EXPR, true);
         fix_fcn_def_params(apo);
      }
      lbp = chunk_get_prev_ncnlni(apo);   // Issue #2279
   }
   else
   {
      lbp = chunk_get_prev_ncnlni(bbo);   // Issue #2279
   }

   // mark the return type, if any
   while (lbp != pc)
   {
      LOG_FMT(LOCBLK, " -- lbp %s[%s]\n", lbp->text(), get_token_name(lbp->type));
      make_type(lbp);
      chunk_flags_set(lbp, PCF_OC_RTYPE);
      set_chunk_parent(lbp, CT_OC_BLOCK_EXPR);
      lbp = chunk_get_prev_ncnlni(lbp);   // Issue #2279
   }
   // mark the braces
   set_chunk_parent(bbo, CT_OC_BLOCK_EXPR);
   set_chunk_parent(bbc, CT_OC_BLOCK_EXPR);
} // handle_oc_block_literal


static void handle_oc_block_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }

   if (pc->flags.test(PCF_IN_TYPEDEF))
   {
      LOG_FMT(LOCBLK, "%s(%d): skip block type @ orig_line is %zu, orig_col is %zu, -- in typedef\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col);
      return;
   }
   // make sure we have '( ^'
   chunk_t *tpo = chunk_get_prev_ncnlni(pc); // type paren open   Issue #2279

   if (chunk_is_paren_open(tpo))
   {
      /*
       * block type: 'RTYPE (^LABEL)(ARGS)'
       * LABEL is optional.
       */
      chunk_t *tpc = chunk_skip_to_match(tpo);   // type close paren (after '^')
      chunk_t *nam = chunk_get_prev_ncnlni(tpc); // name (if any) or '^'   Issue #2279
      chunk_t *apo = chunk_get_next_ncnl(tpc);   // arg open paren
      chunk_t *apc = chunk_skip_to_match(apo);   // arg close paren

      /*
       * If this is a block literal instead of a block type, 'nam'
       * will actually be the closing bracket of the block. We run into
       * this situation if a block literal is enclosed in parentheses.
       */
      if (chunk_is_closing_brace(nam))
      {
         return(handle_oc_block_literal(pc));
      }

      // Check apo is '(' or else this might be a block literal. Issue 2643.
      if (!chunk_is_paren_open(apo))
      {
         return(handle_oc_block_literal(pc));
      }

      if (chunk_is_paren_close(apc))
      {
         chunk_t   *aft = chunk_get_next_ncnl(apc);
         c_token_t pt;

         if (chunk_is_str(nam, "^", 1))
         {
            set_chunk_type(nam, CT_PTR_TYPE);
            pt = CT_FUNC_TYPE;
         }
         else if (  chunk_is_token(aft, CT_ASSIGN)
                 || chunk_is_token(aft, CT_SEMICOLON))
         {
            set_chunk_type(nam, CT_FUNC_VAR);
            pt = CT_FUNC_VAR;
         }
         else
         {
            set_chunk_type(nam, CT_FUNC_TYPE);
            pt = CT_FUNC_TYPE;
         }
         LOG_FMT(LOCBLK, "%s(%d): block type @ orig_line is %zu, orig_col is %zu, text() '%s'[%s]\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, nam->text(), get_token_name(nam->type));
         set_chunk_type(pc, CT_PTR_TYPE);
         set_chunk_parent(pc, pt);  //CT_OC_BLOCK_TYPE;
         set_chunk_type(tpo, CT_TPAREN_OPEN);
         set_chunk_parent(tpo, pt); //CT_OC_BLOCK_TYPE;
         set_chunk_type(tpc, CT_TPAREN_CLOSE);
         set_chunk_parent(tpc, pt); //CT_OC_BLOCK_TYPE;
         set_chunk_type(apo, CT_FPAREN_OPEN);
         set_chunk_parent(apo, CT_FUNC_PROTO);
         set_chunk_type(apc, CT_FPAREN_CLOSE);
         set_chunk_parent(apc, CT_FUNC_PROTO);
         fix_fcn_def_params(apo);
         mark_function_return_type(nam, chunk_get_prev_ncnlni(tpo), pt);   // Issue #2279
      }
   }
} // handle_oc_block_type


static chunk_t *handle_oc_md_type(chunk_t *paren_open, c_token_t ptype, pcf_flags_t flags, bool &did_it)
{
   chunk_t *paren_close;

   if (  !chunk_is_paren_open(paren_open)
      || ((paren_close = chunk_skip_to_match(paren_open)) == nullptr))
   {
      did_it = false;
      return(paren_open);
   }
   did_it = true;

   set_chunk_parent(paren_open, ptype);
   chunk_flags_set(paren_open, flags);
   set_chunk_parent(paren_close, ptype);
   chunk_flags_set(paren_close, flags);

   for (chunk_t *cur = chunk_get_next_ncnl(paren_open);
        cur != paren_close;
        cur = chunk_get_next_ncnl(cur))
   {
      LOG_FMT(LOCMSGD, " <%s|%s>", cur->text(), get_token_name(cur->type));
      chunk_flags_set(cur, flags);
      make_type(cur);
   }

   // returning the chunk after the paren close
   return(chunk_get_next_ncnl(paren_close));
}


static void handle_oc_message_decl(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   bool did_it;
   //bool      in_paren  = false;
   //int       paren_cnt = 0;
   //int       arg_cnt   = 0;

   // Figure out if this is a spec or decl
   chunk_t *tmp = pc;

   while ((tmp = chunk_get_next(tmp)) != nullptr)
   {
      if (tmp->level < pc->level)
      {
         // should not happen
         return;
      }

      if (chunk_is_token(tmp, CT_SEMICOLON) || chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         break;
      }
   }

   if (tmp == nullptr)
   {
      return;
   }
   c_token_t pt = (tmp->type == CT_SEMICOLON) ? CT_OC_MSG_SPEC : CT_OC_MSG_DECL;

   set_chunk_type(pc, CT_OC_SCOPE);
   set_chunk_parent(pc, pt);

   LOG_FMT(LOCMSGD, "%s(%d): %s @ orig_line is %zu, orig_col is %zu -",
           __func__, __LINE__, get_token_name(pt), pc->orig_line, pc->orig_col);

   // format: -(TYPE) NAME [: (TYPE)NAME

   // handle the return type
   tmp = handle_oc_md_type(chunk_get_next_ncnl(pc), pt, PCF_OC_RTYPE, did_it);

   if (!did_it)
   {
      LOG_FMT(LOCMSGD, " -- missing type parens\n");
      return;
   }

   // expect the method name/label
   if (!chunk_is_token(tmp, CT_WORD))
   {
      LOG_FMT(LOCMSGD, " -- missing method name\n");
      return;
   }  // expect the method name/label

   chunk_t *label = tmp;

   set_chunk_type(tmp, pt);
   set_chunk_parent(tmp, pt);
   pc = chunk_get_next_ncnl(tmp);

   LOG_FMT(LOCMSGD, " [%s]%s", pc->text(), get_token_name(pc->type));

   // if we have a colon next, we have args
   if (chunk_is_token(pc, CT_COLON) || chunk_is_token(pc, CT_OC_COLON))
   {
      pc = label;

      while (true)
      {
         // skip optional label
         if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, pt))
         {
            set_chunk_parent(pc, pt);
            pc = chunk_get_next_ncnl(pc);
         }

         // a colon must be next
         if (!chunk_is_str(pc, ":", 1))
         {
            break;
         }
         set_chunk_type(pc, CT_OC_COLON);
         set_chunk_parent(pc, pt);
         pc = chunk_get_next_ncnl(pc);

         // next is the type in parens
         LOG_FMT(LOCMSGD, "  (%s)", pc->text());
         tmp = handle_oc_md_type(pc, pt, PCF_OC_ATYPE, did_it);

         if (!did_it)
         {
            LOG_FMT(LWARN, "%s(%d): orig_line is %zu, orig_col is %zu expected type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col);
            break;
         }
         // attributes for a method parameter sit between the parameter type and the parameter name
         pc = skip_attribute_next(tmp);
         // we should now be on the arg name
         chunk_flags_set(pc, PCF_VAR_DEF);
         LOG_FMT(LOCMSGD, " arg[%s]", pc->text());
         pc = chunk_get_next_ncnl(pc);
      }
   }
   LOG_FMT(LOCMSGD, " end[%s]", pc->text());

   if (chunk_is_token(pc, CT_BRACE_OPEN))
   {
      set_chunk_parent(pc, pt);
      pc = chunk_skip_to_match(pc);

      if (pc != nullptr)
      {
         set_chunk_parent(pc, pt);
      }
   }
   else if (chunk_is_token(pc, CT_SEMICOLON))
   {
      set_chunk_parent(pc, pt);
   }
   LOG_FMT(LOCMSGD, "\n");
} // handle_oc_message_decl


static void handle_oc_message_send(chunk_t *os)
{
   LOG_FUNC_ENTRY();

   chunk_t *cs = chunk_get_next(os);

   while (cs != nullptr && cs->level > os->level)
   {
      cs = chunk_get_next(cs);
   }

   if (cs == nullptr || cs->type != CT_SQUARE_CLOSE)
   {
      return;
   }
   LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, os->orig_line, os->orig_col);

   chunk_t *tmp = chunk_get_next_ncnl(cs);

   if (chunk_is_semicolon(tmp))
   {
      set_chunk_parent(tmp, CT_OC_MSG);
   }
   // expect a word first thing or [...]
   tmp = chunk_get_next_ncnl(os);

   if (  chunk_is_token(tmp, CT_SQUARE_OPEN) || chunk_is_token(tmp, CT_PAREN_OPEN)
      || (chunk_is_token(tmp, CT_OC_AT)))
   {
      chunk_t *tt = chunk_get_next_ncnl(tmp);

      if ((chunk_is_token(tmp, CT_OC_AT)) && tt)
      {
         if (  (chunk_is_token(tt, CT_PAREN_OPEN))
            || (chunk_is_token(tt, CT_BRACE_OPEN))
            || (chunk_is_token(tt, CT_SQUARE_OPEN)))
         {
            tmp = tt;
         }
         else
         {
            LOG_FMT(LOCMSG, "%s(%d): tmp->orig_line is %zu, tmp->orig_col is %zu, expected identifier, not '%s' [%s]\n",
                    __func__, __LINE__, tmp->orig_line, tmp->orig_col,
                    tmp->text(), get_token_name(tmp->type));
            return;
         }
      }
      tmp = chunk_skip_to_match(tmp);
   }
   else if (  tmp->type != CT_WORD
           && tmp->type != CT_TYPE
           && tmp->type != CT_THIS
           && tmp->type != CT_STAR
           && tmp->type != CT_STRING)
   {
      LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu, expected identifier, not '%s' [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col,
              tmp->text(), get_token_name(tmp->type));
      return;
   }
   else
   {
      if (chunk_is_star(tmp)) // Issue #2722
      {
         set_chunk_type(tmp, CT_PTR_TYPE);
         tmp = chunk_get_next_ncnl(tmp);
      }
      chunk_t *tt = chunk_get_next_ncnl(tmp);

      if (chunk_is_paren_open(tt))
      {
         LOG_FMT(LFCN, "%s(%d): (18) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->text());
         set_chunk_type(tmp, CT_FUNC_CALL);
         tmp = chunk_get_prev_ncnlni(set_paren_parent(tt, CT_FUNC_CALL));   // Issue #2279
      }
      else
      {
         set_chunk_type(tmp, CT_OC_MSG_CLASS);
      }
   }
   set_chunk_parent(os, CT_OC_MSG);
   chunk_flags_set(os, PCF_IN_OC_MSG);
   set_chunk_parent(cs, CT_OC_MSG);
   chunk_flags_set(cs, PCF_IN_OC_MSG);

   // handle '< protocol >'
   tmp = chunk_get_next_ncnl(tmp);

   if (chunk_is_str(tmp, "<", 1))
   {
      chunk_t *ao = tmp;
      chunk_t *ac = chunk_get_next_str(ao, ">", 1, ao->level);

      if (ac)
      {
         set_chunk_type(ao, CT_ANGLE_OPEN);
         set_chunk_parent(ao, CT_OC_PROTO_LIST);
         set_chunk_type(ac, CT_ANGLE_CLOSE);
         set_chunk_parent(ac, CT_OC_PROTO_LIST);

         for (tmp = chunk_get_next(ao); tmp != ac; tmp = chunk_get_next(tmp))
         {
            tmp->level += 1;
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }
      }
      tmp = chunk_get_next_ncnl(ac);
   }
   // handle 'object.property' and 'collection[index]'
   else
   {
      while (tmp)
      {
         if (chunk_is_token(tmp, CT_MEMBER))  // move past [object.prop1.prop2
         {
            chunk_t *typ = chunk_get_next_ncnl(tmp);

            if (chunk_is_token(typ, CT_WORD) || chunk_is_token(typ, CT_TYPE))
            {
               tmp = chunk_get_next_ncnl(typ);
            }
            else
            {
               break;
            }
         }
         else if (chunk_is_token(tmp, CT_SQUARE_OPEN))  // move past [collection[index]
         {
            chunk_t *tcs = chunk_get_next_ncnl(tmp);

            while (tcs != nullptr && tcs->level > tmp->level)
            {
               tcs = chunk_get_next_ncnl(tcs);
            }

            if (chunk_is_token(tcs, CT_SQUARE_CLOSE))
            {
               tmp = chunk_get_next_ncnl(tcs);
            }
            else
            {
               break;
            }
         }
         else
         {
            break;
         }
      }
   }

   // [(self.foo.bar) method]
   if (chunk_is_paren_open(tmp))
   {
      tmp = chunk_get_next_ncnl(chunk_skip_to_match(tmp));
   }

   if (chunk_is_token(tmp, CT_WORD) || chunk_is_token(tmp, CT_TYPE))
   {
      set_chunk_type(tmp, CT_OC_MSG_FUNC);
   }
   chunk_t *prev = nullptr;

   for (tmp = chunk_get_next(os); tmp != cs; tmp = chunk_get_next(tmp))
   {
      chunk_flags_set(tmp, PCF_IN_OC_MSG);

      if (tmp->level == cs->level + 1)
      {
         if (chunk_is_token(tmp, CT_COLON))
         {
            set_chunk_type(tmp, CT_OC_COLON);

            if (chunk_is_token(prev, CT_WORD) || chunk_is_token(prev, CT_TYPE))
            {
               // Might be a named param, check previous block
               chunk_t *pp = chunk_get_prev(prev);

               if (  pp != nullptr
                  && pp->type != CT_OC_COLON
                  && pp->type != CT_ARITH
                  && pp->type != CT_CARET)
               {
                  set_chunk_type(prev, CT_OC_MSG_NAME);
                  set_chunk_parent(tmp, CT_OC_MSG_NAME);
               }
            }
         }
      }
      prev = tmp;
   }
} // handle_oc_message_send


static void handle_oc_available(chunk_t *os)
{
   os = chunk_get_next(os);

   while (os != nullptr)
   {
      c_token_t origType = os->type;
      set_chunk_type(os, CT_OC_AVAILABLE_VALUE);

      if (origType == CT_PAREN_CLOSE)
      {
         break;
      }
      os = chunk_get_next(os);
   }
}


static void handle_oc_property_decl(chunk_t *os)
{
   log_rule_B("mod_sort_oc_properties");

   if (options::mod_sort_oc_properties())
   {
      typedef std::vector<chunk_t *> ChunkGroup;

      chunk_t                 *next       = chunk_get_next(os);
      chunk_t                 *open_paren = nullptr;

      std::vector<ChunkGroup> class_chunks;       // class
      std::vector<ChunkGroup> thread_chunks;      // atomic, nonatomic
      std::vector<ChunkGroup> readwrite_chunks;   // readwrite, readonly
      std::vector<ChunkGroup> ref_chunks;         // retain, copy, assign, weak, strong, unsafe_unretained
      std::vector<ChunkGroup> getter_chunks;      // getter
      std::vector<ChunkGroup> setter_chunks;      // setter
      std::vector<ChunkGroup> nullability_chunks; // nonnull, nullable, null_unspecified, null_resettable
      std::vector<ChunkGroup> other_chunks;       // any words other than above

      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         open_paren = next;
         next       = chunk_get_next(next);

         /*
          * Determine location of the property attributes
          * NOTE: Did not do this in the combine.cpp do_symbol_check as
          * I was not sure what the ramifications of adding a new type
          * for each of the below types would be. It did break some items
          * when I attempted to add them so this is my hack for now.
          */
         while (next != nullptr && next->type != CT_PAREN_CLOSE)
         {
            if (chunk_is_token(next, CT_OC_PROPERTY_ATTR))
            {
               if (  chunk_is_str(next, "atomic", 6)
                  || chunk_is_str(next, "nonatomic", 9))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  thread_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "readonly", 8)
                       || chunk_is_str(next, "readwrite", 9))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  readwrite_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "assign", 6)
                       || chunk_is_str(next, "retain", 6)
                       || chunk_is_str(next, "copy", 4)
                       || chunk_is_str(next, "strong", 6)
                       || chunk_is_str(next, "weak", 4)
                       || chunk_is_str(next, "unsafe_unretained", 17))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  ref_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "getter", 6))
               {
                  ChunkGroup chunkGroup;

                  do
                  {
                     chunkGroup.push_back(next);
                     next = chunk_get_next(next);
                  } while (  next
                          && next->type != CT_COMMA
                          && next->type != CT_PAREN_CLOSE);

                  next = next->prev;

                  // coverity CID 160946
                  if (next == nullptr)
                  {
                     break;
                  }
                  getter_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "setter", 6))
               {
                  ChunkGroup chunkGroup;

                  do
                  {
                     chunkGroup.push_back(next);
                     next = chunk_get_next(next);
                  } while (  next
                          && next->type != CT_COMMA
                          && next->type != CT_PAREN_CLOSE);

                  next = chunk_get_prev(next);

                  if (next == nullptr)
                  {
                     break;
                  }
                  setter_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "nullable", 8)
                       || chunk_is_str(next, "nonnull", 7)
                       || chunk_is_str(next, "null_resettable", 15)
                       || chunk_is_str(next, "null_unspecified", 16))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  nullability_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "class", 5))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  class_chunks.push_back(chunkGroup);
               }
               else
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  other_chunks.push_back(chunkGroup);
               }
            }
            else if (chunk_is_word(next))
            {
               if (chunk_is_str(next, "class", 5))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  class_chunks.push_back(chunkGroup);
               }
               else
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  other_chunks.push_back(chunkGroup);
               }
            }
            next = chunk_get_next(next);
         }
         log_rule_B("mod_sort_oc_property_class_weight");
         int class_w = options::mod_sort_oc_property_class_weight();
         log_rule_B("mod_sort_oc_property_thread_safe_weight");
         int thread_w = options::mod_sort_oc_property_thread_safe_weight();
         log_rule_B("mod_sort_oc_property_readwrite_weight");
         int readwrite_w = options::mod_sort_oc_property_readwrite_weight();
         log_rule_B("mod_sort_oc_property_reference_weight");
         int ref_w = options::mod_sort_oc_property_reference_weight();
         log_rule_B("mod_sort_oc_property_getter_weight");
         int getter_w = options::mod_sort_oc_property_getter_weight();
         log_rule_B("mod_sort_oc_property_setter_weight");
         int setter_w = options::mod_sort_oc_property_setter_weight();
         log_rule_B("mod_sort_oc_property_nullability_weight");
         int nullability_w = options::mod_sort_oc_property_nullability_weight();

         //
         std::multimap<int, std::vector<ChunkGroup> > sorted_chunk_map;
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(class_w, class_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(thread_w, thread_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(readwrite_w, readwrite_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(ref_w, ref_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(getter_w, getter_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(setter_w, setter_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(nullability_w, nullability_chunks));
         sorted_chunk_map.insert(pair<int, std::vector<ChunkGroup> >(std::numeric_limits<int>::min(), other_chunks));

         chunk_t *curr_chunk = open_paren;

         for (multimap<int, std::vector<ChunkGroup> >::reverse_iterator it = sorted_chunk_map.rbegin(); it != sorted_chunk_map.rend(); ++it)
         {
            std::vector<ChunkGroup> chunk_groups = (*it).second;

            for (auto chunk_group : chunk_groups)
            {
               for (auto chunk : chunk_group)
               {
                  chunk->orig_prev_sp = 0;

                  if (chunk != curr_chunk)
                  {
                     chunk_move_after(chunk, curr_chunk);
                     curr_chunk = chunk;
                  }
                  else
                  {
                     curr_chunk = chunk_get_next(curr_chunk);
                  }
               }

               // add the parenthesis
               chunk_t endchunk;
               set_chunk_type(&endchunk, CT_COMMA);
               set_chunk_parent(&endchunk, get_chunk_parent_type(curr_chunk));
               endchunk.str         = ",";
               endchunk.level       = curr_chunk->level;
               endchunk.brace_level = curr_chunk->brace_level;
               endchunk.orig_line   = curr_chunk->orig_line;
               endchunk.orig_col    = curr_chunk->orig_col;
               endchunk.column      = curr_chunk->orig_col_end + 1;
               endchunk.flags       = curr_chunk->flags & PCF_COPY_FLAGS;
               chunk_add_after(&endchunk, curr_chunk);
               curr_chunk = curr_chunk->next;
            }
         }

         // Remove the extra comma's that we did not move
         while (curr_chunk && curr_chunk->type != CT_PAREN_CLOSE)
         {
            chunk_t *rm_chunk = curr_chunk;
            curr_chunk = chunk_get_next(curr_chunk);
            chunk_del(rm_chunk);
         }
      }
   }
   chunk_t *tmp = chunk_get_next_ncnl(os);

   if (chunk_is_paren_open(tmp))
   {
      tmp = chunk_get_next_ncnl(chunk_skip_to_match(tmp));
   }
   fix_variable_definition(tmp);
} // handle_oc_property_decl


static void handle_cs_square_stmt(chunk_t *os)
{
   LOG_FUNC_ENTRY();

   chunk_t *cs = chunk_get_next(os);

   while (cs != nullptr && cs->level > os->level)
   {
      cs = chunk_get_next(cs);
   }

   if (cs == nullptr || cs->type != CT_SQUARE_CLOSE)
   {
      return;
   }
   set_chunk_parent(os, CT_CS_SQ_STMT);
   set_chunk_parent(cs, CT_CS_SQ_STMT);

   chunk_t *tmp;

   for (tmp = chunk_get_next(os); tmp != cs; tmp = chunk_get_next(tmp))
   {
      set_chunk_parent(tmp, CT_CS_SQ_STMT);

      if (chunk_is_token(tmp, CT_COLON))
      {
         set_chunk_type(tmp, CT_CS_SQ_COLON);
      }
   }

   tmp = chunk_get_next_ncnl(cs);

   if (tmp != nullptr)
   {
      chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
   }
}


static void handle_cs_property(chunk_t *bro)
{
   LOG_FUNC_ENTRY();

   set_paren_parent(bro, CT_CS_PROPERTY);

   bool    did_prop = false;
   chunk_t *pc      = bro;

   while ((pc = chunk_get_prev_ncnlni(pc)) != nullptr)   // Issue #2279
   {
      if (pc->level == bro->level)
      {
         //prevent scanning back past 'new' in expressions like new List<int> {1,2,3}
         // Issue # 1620, UNI-24090.cs
         if (chunk_is_token(pc, CT_NEW))
         {
            break;
         }

         if (  !did_prop
            && (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_THIS)))
         {
            set_chunk_type(pc, CT_CS_PROPERTY);
            did_prop = true;
         }
         else
         {
            set_chunk_parent(pc, CT_CS_PROPERTY);
            make_type(pc);
         }

         if (pc->flags.test(PCF_STMT_START))
         {
            break;
         }
      }
   }
}


static void handle_cs_array_type(chunk_t *pc)
{
   chunk_t *prev;

   for (prev = chunk_get_prev(pc);
        chunk_is_token(prev, CT_COMMA);
        prev = chunk_get_prev(prev))
   {
      // empty
   }

   if (chunk_is_token(prev, CT_SQUARE_OPEN))
   {
      while (pc != prev)
      {
         set_chunk_parent(pc, CT_TYPE);
         pc = chunk_get_prev(pc);
      }
      set_chunk_parent(prev, CT_TYPE);
   }
}


static void handle_wrap(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *opp  = chunk_get_next(pc);
   chunk_t *name = chunk_get_next(opp);
   chunk_t *clp  = chunk_get_next(name);

   log_rule_B("sp_func_call_paren");
   log_rule_B("sp_cpp_cast_paren");
   iarf_e pav = (pc->type == CT_FUNC_WRAP) ?
                options::sp_func_call_paren() :
                options::sp_cpp_cast_paren();

   log_rule_B("sp_inside_fparen");
   log_rule_B("sp_inside_paren_cast");
   iarf_e av = (pc->type == CT_FUNC_WRAP) ?
               options::sp_inside_fparen() :
               options::sp_inside_paren_cast();

   if (  chunk_is_token(clp, CT_PAREN_CLOSE)
      && chunk_is_token(opp, CT_PAREN_OPEN)
      && (chunk_is_token(name, CT_WORD) || chunk_is_token(name, CT_TYPE)))
   {
      const char *psp = (pav & IARF_ADD) ? " " : "";
      const char *fsp = (av & IARF_ADD) ? " " : "";

      pc->str.append(psp);
      pc->str.append("(");
      pc->str.append(fsp);
      pc->str.append(name->str);
      pc->str.append(fsp);
      pc->str.append(")");

      set_chunk_type(pc, (pc->type == CT_FUNC_WRAP) ? CT_FUNCTION : CT_TYPE);

      pc->orig_col_end = pc->orig_col + pc->len();

      chunk_del(opp);
      chunk_del(name);
      chunk_del(clp);
   }
} // handle_wrap


static void handle_proto_wrap(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *opp  = chunk_get_next_ncnl(pc);
   chunk_t *name = chunk_get_next_ncnl(opp);
   chunk_t *tmp  = chunk_get_next_ncnl(chunk_get_next_ncnl(name));
   chunk_t *clp  = chunk_skip_to_match(opp);
   chunk_t *cma  = chunk_get_next_ncnl(clp);

   if (  !opp
      || !name
      || !clp
      || !cma
      || !tmp
      || (name->type != CT_WORD && name->type != CT_TYPE)
      || opp->type != CT_PAREN_OPEN)
   {
      return;
   }

   if (chunk_is_token(cma, CT_SEMICOLON))
   {
      set_chunk_type(pc, CT_FUNC_PROTO);
   }
   else if (chunk_is_token(cma, CT_BRACE_OPEN))
   {
      LOG_FMT(LFCN, "%s(%d): (19) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      set_chunk_type(pc, CT_FUNC_DEF);
   }
   else
   {
      return;
   }
   set_chunk_parent(opp, pc->type);
   set_chunk_parent(clp, pc->type);

   set_chunk_parent(tmp, CT_PROTO_WRAP);

   if (chunk_is_token(tmp, CT_PAREN_OPEN))
   {
      fix_fcn_def_params(tmp);
   }
   else
   {
      fix_fcn_def_params(opp);
      set_chunk_type(name, CT_WORD);
   }
   tmp = chunk_skip_to_match(tmp);

   if (tmp)
   {
      set_chunk_parent(tmp, CT_PROTO_WRAP);
   }
   // Mark return type (TODO: move to own function)
   tmp = pc;

   while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
   {
      if (  !chunk_is_type(tmp)
         && tmp->type != CT_OPERATOR
         && tmp->type != CT_WORD
         && tmp->type != CT_ADDR)
      {
         break;
      }
      set_chunk_parent(tmp, pc->type);
      make_type(tmp);
   }
} // handle_proto_wrap


/**
 * Java assert statements are: "assert EXP1 [: EXP2] ;"
 * Mark the parent of the colon and semicolon
 */
static void handle_java_assert(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   bool    did_colon = false;
   chunk_t *tmp      = pc;

   while ((tmp = chunk_get_next(tmp)) != nullptr)
   {
      if (tmp->level == pc->level)
      {
         if (!did_colon && chunk_is_token(tmp, CT_COLON))
         {
            did_colon = true;
            set_chunk_parent(tmp, pc->type);
         }

         if (chunk_is_token(tmp, CT_SEMICOLON))
         {
            set_chunk_parent(tmp, pc->type);
            break;
         }
      }
   }
}
