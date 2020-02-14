/**
 * @file combine.cpp
 * Labels the chunks as needed.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "combine.h"

#include "chunk_list.h"
#include "combine_labels.h"
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


//! Scan backwards to see if we might be on a type declaration
static bool chunk_ends_type(chunk_t *start);


//! Skips to the start of the next statement.
static chunk_t *skip_to_next_statement(chunk_t *pc);


/**
 * Skips everything until a comma or semicolon at the same level.
 * Returns the semicolon, comma, or close brace/paren or nullptr.
 */
static chunk_t *skip_expression(chunk_t *start);


/**
 * Skips the D 'align()' statement and the colon, if present.
 *    align(2) int foo;  -- returns 'int'
 *    align(4):          -- returns 'int'
 *    int bar;
 */
static chunk_t *skip_align(chunk_t *start);


/**
 * Skips the list of class/struct parent types.
 */
chunk_t *skip_parent_types(chunk_t *colon);


/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
static void check_double_brace_init(chunk_t *bo1);


/**
 * Simply change any STAR to PTR_TYPE and WORD to TYPE
 *
 * @param start  points to the open paren
 */
static void fix_fcn_def_params(chunk_t *pc);


/**
 * We are on a typedef.
 * If the next word is not enum/union/struct, then the last word before the
 * next ',' or ';' or '__attribute__' is a type.
 *
 * typedef [type...] [*] type [, [*]type] ;
 * typedef <return type>([*]func)();
 * typedef <return type>([*]func)(params);
 * typedef <return type>(__stdcall *func)(); Bug # 633    MS-specific extension
 *                                           include the config-file "test/config/MS-calling_conventions.cfg"
 * typedef <return type>func(params);
 * typedef <enum/struct/union> [type] [*] type [, [*]type] ;
 * typedef <enum/struct/union> [type] { ... } [*] type [, [*]type] ;
 */
static void fix_typedef(chunk_t *pc);


/**
 * We are on an enum/struct/union tag that is NOT inside a typedef.
 * If there is a {...} and words before the ';', then they are variables.
 *
 * tag { ... } [*] word [, [*]word] ;
 * tag [word/type] { ... } [*] word [, [*]word] ;
 * enum [word/type [: int_type]] { ... } [*] word [, [*]word] ;
 * tag [word/type] [word]; -- this gets caught later.
 * fcn(tag [word/type] [word])
 * a = (tag [word/type] [*])&b;
 *
 * REVISIT: should this be consolidated with the typedef code?
 */
static void fix_enum_struct_union(chunk_t *pc);


/**
 * Checks to see if the current paren is part of a cast.
 * We already verified that this doesn't follow function, TYPE, IF, FOR,
 * SWITCH, or WHILE and is followed by WORD, TYPE, STRUCT, ENUM, or UNION.
 *
 * @param start  Pointer to the open paren
 */
static void fix_casts(chunk_t *pc);


/**
 * CT_TYPE_CAST follows this pattern:
 * dynamic_cast<...>(...)
 *
 * Mark everything between the <> as a type and set the paren parent
 */
static void fix_type_cast(chunk_t *pc);


/**
 * We are on the start of a sequence that could be a var def
 *  - FPAREN_OPEN (parent == CT_FOR)
 *  - BRACE_OPEN
 *  - SEMICOLON
 */
static chunk_t *fix_var_def(chunk_t *pc);


/**
 * We are on a function word. we need to:
 *  - find out if this is a call or prototype or implementation
 *  - mark return type
 *  - mark parameter types
 *  - mark brace pair
 *
 * REVISIT:
 * This whole function is a mess.
 * It needs to be reworked to eliminate duplicate logic and determine the
 * function type more directly.
 *  1. Skip to the close paren and see what is after.
 *     a. semicolon - function call or function proto
 *     b. open brace - function call (ie, list_for_each) or function def
 *     c. open paren - function type or chained function call
 *     d. qualifier - function def or proto, continue to semicolon or open brace
 *  2. Examine the 'parameters' to see if it can be a proto/def
 *  3. Examine what is before the function name to see if it is a proto or call
 * Constructor/destructor detection should have already been done when the
 * 'class' token was encountered (see mark_class_ctor).
 */
static void mark_function(chunk_t *pc);


/**
 * Checks to see if a series of chunks could be a C++ parameter
 * FOO foo(5, &val);
 *
 * WORD means CT_WORD or CT_TYPE
 *
 * "WORD WORD"          ==> true
 * "QUALIFIER ??"       ==> true
 * "TYPE"               ==> true
 * "WORD"               ==> true
 * "WORD.WORD"          ==> true
 * "WORD::WORD"         ==> true
 * "WORD * WORD"        ==> true
 * "WORD & WORD"        ==> true
 * "NUMBER"             ==> false
 * "STRING"             ==> false
 * "OPEN PAREN"         ==> false
 *
 * @param start  the first chunk to look at
 * @param end    the chunk after the last one to look at
 */
static bool can_be_full_param(chunk_t *start, chunk_t *end);


/**
 * Changes the return type to type and set the parent.
 *
 * @param pc           the last chunk of the return type
 * @param parent_type  CT_NONE (no change) or the new parent type
 */
static void mark_function_return_type(chunk_t *fname, chunk_t *pc, c_token_t parent_type);


/**
 * Process a function type that is not in a typedef.
 * pc points to the first close paren.
 *
 * void (*func)(params);
 * const char * (*func)(params);
 * const char * (^func)(params);   -- Objective C
 *
 * @param pc  Points to the first closing paren
 *
 * @return whether a function type was processed
 */
static bool mark_function_type(chunk_t *pc);


/**
 * Examines the stuff between braces { }.
 * There should only be variable definitions and methods.
 * Skip the methods, as they will get handled elsewhere.
 */
static void mark_struct_union_body(chunk_t *start);


/**
 * We are on the first word of a variable definition.
 * Mark all the variable names with PCF_VAR_1ST and PCF_VAR_DEF as appropriate.
 * Also mark any '*' encountered as a CT_PTR_TYPE.
 * Skip over []. Go until a ';' is hit.
 *
 * Example input:
 * int   a = 3, b, c = 2;              ## called with 'a'
 * foo_t f = {1, 2, 3}, g = {5, 6, 7}; ## called with 'f'
 * struct {...} *a, *b;                ## called with 'a' or '*'
 * myclass a(4);
 */
static chunk_t *mark_variable_definition(chunk_t *start);


/**
 * Marks statement starts in a macro body.
 * REVISIT: this may already be done
 */
static void mark_define_expressions(void);


static void process_returns(void);


/**
 * Processes a return statement, labeling the parens and marking the parent.
 * May remove or add parens around the return statement
 *
 * @param pc  Pointer to the return chunk
 */
static chunk_t *process_return(chunk_t *pc);


/**
 * TODO: add doc cmt
 *
 */
static pcf_flags_t mark_where_chunk(chunk_t *pc, c_token_t parent_type, pcf_flags_t flags);


/**
 * We're on a 'class' or 'struct'.
 * Scan for CT_FUNCTION with a string that matches pclass->str
 */
static void mark_class_ctor(chunk_t *pclass);


static void mark_cpp_constructor(chunk_t *pc);


/**
 *  Just hit an assign. Go backwards until we hit an open brace/paren/square or
 * semicolon (TODO: other limiter?) and mark as a LValue.
 */
static void mark_lvalue(chunk_t *pc);


/**
 * We are on a word followed by a angle open which is part of a template.
 * If the angle close is followed by a open paren, then we are on a template
 * function def or a template function call:
 *   Vector2<float>(...) [: ...[, ...]] { ... }
 * Or we could be on a variable def if it's followed by a word:
 *   Renderer<rgb32> rend;
 */
static void mark_template_func(chunk_t *pc, chunk_t *pc_next);


/**
 * Just mark every CT_WORD until a semicolon as CT_SQL_WORD.
 * Adjust the levels if pc is CT_SQL_BEGIN
 */
static void mark_exec_sql(chunk_t *pc);


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


/**
 * Parse off the types in the D template args, adds to cs
 * returns the close_paren
 */
static chunk_t *get_d_template_types(ChunkStack &cs, chunk_t *open_paren);


static bool chunkstack_match(ChunkStack &cs, chunk_t *pc);


void make_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_WORD))
      {
         set_chunk_type(pc, CT_TYPE);
      }
      else if (chunk_is_star(pc) || chunk_is_msref(pc) || chunk_is_nullable(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (  chunk_is_addr(pc)
              && !chunk_is_token(pc->prev, CT_SQUARE_OPEN))            // Issue # 2166
      {
         set_chunk_type(pc, CT_BYREF);
      }
   }
}


void flag_series(chunk_t *start, chunk_t *end, pcf_flags_t set_flags, pcf_flags_t clr_flags, scope_e nav)
{
   LOG_FUNC_ENTRY();

   while (start != nullptr && start != end)
   {
      chunk_flags_upd(start, clr_flags, set_flags);

      start = chunk_get_next(start, nav);

      if (start == nullptr)
      {
         return;
      }
   }

   if (end != nullptr)
   {
      chunk_flags_upd(end, clr_flags, set_flags);
   }
}


chunk_t *set_paren_parent(chunk_t *start, c_token_t parent)
{
   LOG_FUNC_ENTRY();
   chunk_t *end;

   end = chunk_skip_to_match(start, scope_e::PREPROC);

   if (end != nullptr)
   {
      LOG_FMT(LFLPAREN, "%s(%d): %zu:%zu '%s' and %zu:%zu '%s' type is %s, parent_type is %s",
              __func__, __LINE__, start->orig_line, start->orig_col, start->text(),
              end->orig_line, end->orig_col, end->text(),
              get_token_name(start->type), get_token_name(parent));
      log_func_stack_inline(LFLPAREN);
      set_chunk_parent(start, parent);
      set_chunk_parent(end, parent);
   }
   LOG_FMT(LFLPAREN, "%s(%d):\n", __func__, __LINE__);
   return(chunk_get_next_ncnl(end, scope_e::PREPROC));
}


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

   for (tmp = chunk_get_next_ncnl(po, scope_e::PREPROC);
        tmp != nullptr && tmp != end;
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


static bool chunk_ends_type(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc       = start;
   bool    ret       = false;
   size_t  cnt       = 0;
   bool    last_expr = false;
   bool    last_lval = false;

   for ( ; pc != nullptr; pc = chunk_get_prev_ncnlni(pc)) // Issue #2279
   {
      LOG_FMT(LFTYPE, "%s(%d): type is %s, text() '%s', orig_line %zu, orig_col %zu\n   ",
              __func__, __LINE__, get_token_name(pc->type), pc->text(),
              pc->orig_line, pc->orig_col);
      log_pcf_flags(LFTYPE, pc->flags);

      if (  chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_PTR_TYPE)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_DC_MEMBER)
         || chunk_is_token(pc, CT_PP)
         || chunk_is_token(pc, CT_QUALIFIER)
         || (language_is_set(LANG_CS) && (chunk_is_token(pc, CT_MEMBER))))
      {
         cnt++;
         last_expr = pc->flags.test(PCF_EXPR_START) && !pc->flags.test(PCF_IN_FCN_CALL);
         last_lval = pc->flags.test(PCF_LVALUE);
         continue;
      }

      if (  (chunk_is_semicolon(pc) && !pc->flags.test(PCF_IN_FOR))
         || chunk_is_token(pc, CT_TYPEDEF)
         || chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_VBRACE_CLOSE)
         || chunk_is_token(pc, CT_FPAREN_CLOSE)
         || chunk_is_forin(pc)
         || chunk_is_token(pc, CT_MACRO)
         || chunk_is_token(pc, CT_PP_IF)
         || chunk_is_token(pc, CT_PP_ELSE)
         || chunk_is_token(pc, CT_PP_ENDIF)
         || ((chunk_is_token(pc, CT_COMMA) && !pc->flags.test(PCF_IN_FCN_CALL)) && last_expr)
         || (chunk_is_token(pc, CT_SPAREN_OPEN) && last_lval))
      {
         ret = cnt > 0;
      }
      break;
   }

   if (pc == nullptr)
   {
      // first token
      ret = true;
   }
   LOG_FMT(LFTYPE, "%s(%d): first token verdict: %s\n", __func__, __LINE__, ret ? "yes" : "no");

   return(ret);
} // chunk_ends_type


void do_symbol_check(chunk_t *prev, chunk_t *pc, chunk_t *next)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   // separate the uses of CT_ASSIGN sign '='
   // into CT_ASSIGN_DEFAULT_ARG, CT_ASSIGN_FUNC_PROTO
   if (  chunk_is_token(pc, CT_ASSIGN)
      && get_chunk_parent_type(pc) == CT_FUNC_PROTO
      && (  pc->flags.test(PCF_IN_FCN_DEF) // Issue #2236
         || pc->flags.test(PCF_IN_CONST_ARGS)))
   {
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
      log_pcf_flags(LFCNR, pc->flags);
      set_chunk_type(pc, CT_ASSIGN_DEFAULT_ARG);
   }

   if (  (  chunk_is_token(prev, CT_FPAREN_CLOSE)
         || (  (chunk_is_str(prev, "const", 5) || chunk_is_str(prev, "override", 8))
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
      if (chunk_is_token(pc, CT_D_CAST) && tmp != nullptr)
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

      if (chunk_is_token(pc, CT_ALIGN) && tmp != nullptr)
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
      && (chunk_is_token(pc, CT_GETSET) || chunk_is_token(pc, CT_GETSET_EMPTY)))
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
         if (pc->flags.test(PCF_EXPR_START) || pc->flags.test(PCF_IN_PREPROC))
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
         if ((  chunk_is_str(pc, "-", 1)
             || chunk_is_str(pc, "+", 1)) && chunk_is_str(next, "(", 1))
         {
            handle_oc_message_decl(pc);
         }
      }

      if (pc->flags.test(PCF_EXPR_START) || pc->flags.test(PCF_IN_PREPROC))
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
      if (pc->flags.test(PCF_EXPR_START) && chunk_is_token(pc, CT_SQUARE_OPEN))
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

      if (chunk_is_token(pc, CT_SQUARE_CLOSE) && chunk_is_token(next, CT_WORD))
      {
         handle_cs_array_type(pc);
      }

      if (  ((chunk_is_token(pc, CT_LAMBDA) || chunk_is_token(pc, CT_DELEGATE)))
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

      if ((chunk_is_token(tmp, CT_BRACE_OPEN) || chunk_is_token(tmp, CT_PAREN_OPEN)))
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
      && (chunk_is_token(pc, CT_SQUARE_OPEN) || chunk_is_token(pc, CT_TSQUARE)))
   {
      handle_cpp_lambda(pc);
   }

   // FIXME: which language does this apply to?
   if (chunk_is_token(pc, CT_ASSIGN) && chunk_is_token(next, CT_SQUARE_OPEN))
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
      || (chunk_is_token(pc, CT_CLASS) && !language_is_set(LANG_D)))
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

   if (chunk_is_token(pc, CT_SQUARE_CLOSE) && chunk_is_token(next, CT_PAREN_OPEN))
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

      if ((language_is_set(LANG_C | LANG_CPP | LANG_OC)) && chunk_is_token(tmp, CT_CARET))
      {
         handle_oc_block_type(tmp);

         // This is the case where a block literal is passed as the first argument of a C-style method invocation.
         if ((chunk_is_token(tmp, CT_OC_BLOCK_CARET) || chunk_is_token(tmp, CT_CARET)) && chunk_is_token(pc, CT_WORD))
         {
            set_chunk_type(pc, CT_FUNC_CALL);
         }
      }
      else if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_OPERATOR_VAL))
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
                  if (get_chunk_parent_type(pc) == CT_NONE && !pc->flags.test(PCF_IN_TYPEDEF))
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
      if (chunk_is_token(pc, CT_FUNCTION) && pc->brace_level > 0)
      {
         set_chunk_type(pc, CT_FUNC_CALL);
      }

      if (chunk_is_token(pc, CT_STATE) && chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_paren_parent(next, pc->type);
      }
   }
   else
   {
      if (  (chunk_is_token(pc, CT_FUNCTION) || chunk_is_token(pc, CT_FUNC_DEF))
         && (get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR || !is_oc_block(pc)))
      {
         mark_function(pc);
      }
   }

   // Detect C99 member stuff
   if (  chunk_is_token(pc, CT_MEMBER)
      && (chunk_is_token(prev, CT_COMMA) || chunk_is_token(prev, CT_BRACE_OPEN)))
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
         || chunk_is_token(pc, CT_COLON)
         || (  chunk_is_token(pc, CT_BRACE_OPEN)
            && (  get_chunk_parent_type(pc) == CT_NONE
               || get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)))
      {
         auto brace_open = chunk_get_next_ncnl(pc);

         if (  chunk_is_token(brace_open, CT_BRACE_OPEN)
            && (  get_chunk_parent_type(brace_open) == CT_NONE
               || get_chunk_parent_type(brace_open) == CT_ASSIGN
               || get_chunk_parent_type(brace_open) == CT_RETURN
               || get_chunk_parent_type(brace_open) == CT_BRACED_INIT_LIST))
         {
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
      /*
       * A variable definition is possible after at the start of a statement
       * that starts with: DC_MEMBER, QUALIFIER, TYPE, or WORD
       */
      // Issue #2279
      // Issue #2478
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, text() is '%s', type is %s, parent_type is %s\n",
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
         pc = fix_var_def(pc);
      }
      else
      {
         pc = chunk_get_next_ncnl(pc);
      }
   }
} // fix_symbols


static void mark_lvalue(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev;

   if (pc->flags.test(PCF_IN_PREPROC))
   {
      return;
   }

   for (prev = chunk_get_prev_ncnlni(pc);     // Issue #2279
        prev != nullptr;
        prev = chunk_get_prev_ncnlni(prev))   // Issue #2279
   {
      if (  prev->level < pc->level
         || chunk_is_token(prev, CT_ASSIGN)
         || chunk_is_token(prev, CT_COMMA)
         || chunk_is_token(prev, CT_BOOL)
         || chunk_is_semicolon(prev)
         || chunk_is_str(prev, "(", 1)
         || chunk_is_str(prev, "{", 1)
         || chunk_is_str(prev, "[", 1)
         || prev->flags.test(PCF_IN_PREPROC))
      {
         break;
      }
      chunk_flags_set(prev, PCF_LVALUE);

      if (prev->level == pc->level && chunk_is_str(prev, "&", 1))
      {
         make_type(prev);
      }
   }
}


static void mark_function_return_type(chunk_t *fname, chunk_t *start, c_token_t parent_type)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   if (pc != nullptr)
   {
      // Step backwards from pc and mark the parent of the return type
      LOG_FMT(LFCNR, "%s(%d): (backwards) return type for '%s' @ orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, fname->text(), fname->orig_line, fname->orig_col);

      chunk_t *first = pc;

      while (pc != nullptr)
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s, ",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         log_pcf_flags(LFCNR, pc->flags);

         if (chunk_is_token(pc, CT_ANGLE_CLOSE))
         {
            pc = skip_template_prev(pc);

            if (pc == nullptr || chunk_is_token(pc, CT_TEMPLATE))
            {
               //either expression is not complete or this is smth like 'template<T> void func()'
               //  - we are not interested in 'template<T>' part
               break;
            }
            else
            {
               //this is smth like 'vector<int> func()' and 'pc' is currently on 'vector' - just proceed
            }
         }

         if (  (  !chunk_is_type(pc)
               && pc->type != CT_OPERATOR
               && pc->type != CT_WORD
               && pc->type != CT_ADDR)
            || pc->flags.test(PCF_IN_PREPROC))
         {
            break;
         }

         if (!chunk_is_ptr_operator(pc))
         {
            first = pc;
         }
         pc = chunk_get_prev_ncnlni(pc);   // Issue #2279
      }
      LOG_FMT(LFCNR, "%s(%d): marking returns...", __func__, __LINE__);

      // Changing words to types into tuple return types in CS.
      bool is_return_tuple = false;

      if (chunk_is_token(pc, CT_PAREN_CLOSE) && !pc->flags.test(PCF_IN_PREPROC))
      {
         first           = chunk_skip_to_match_rev(pc);
         is_return_tuple = true;
      }
      pc = first;

      while (pc != nullptr)
      {
         LOG_FMT(LFCNR, " text() '%s', type is %s", pc->text(), get_token_name(pc->type));

         if (parent_type != CT_NONE)
         {
            set_chunk_parent(pc, parent_type);
         }
         chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279

         if (  !is_return_tuple || pc->type != CT_WORD
            || (prev != nullptr && prev->type != CT_TYPE))
         {
            make_type(pc);
         }

         if (pc == start)
         {
            break;
         }
         pc = chunk_get_next_ncnl(pc);

         //template angles should keep parent type CT_TEMPLATE
         if (chunk_is_token(pc, CT_ANGLE_OPEN))
         {
            pc = chunk_get_next_type(pc, CT_ANGLE_CLOSE, pc->level);

            if (pc == start)
            {
               break;
            }
            pc = chunk_get_next_ncnl(pc);
         }
      }
      LOG_FMT(LFCNR, "\n");

      // Back up and mark parent type on friend declarations
      if (parent_type != CT_NONE && first && first->flags.test(PCF_IN_CLASS))
      {
         pc = chunk_get_prev_ncnlni(first);   // Issue #2279

         if (chunk_is_token(pc, CT_FRIEND))
         {
            LOG_FMT(LFCNR, "%s(%d): marking friend\n", __func__, __LINE__);
            set_chunk_parent(pc, parent_type);
            // A friend might be preceded by a template specification, as in:
            //   template <...> friend type func(...);
            // If so, we need to mark that also
            pc = chunk_get_prev_ncnlni(pc);   // Issue #2279

            if (chunk_is_token(pc, CT_ANGLE_CLOSE))
            {
               pc = skip_template_prev(pc);

               if (chunk_is_token(pc, CT_TEMPLATE))
               {
                  LOG_FMT(LFCNR, "%s(%d): marking friend template\n",
                          __func__, __LINE__);
                  set_chunk_parent(pc, parent_type);
               }
            }
         }
      }
   }
} // mark_function_return_type


static bool mark_function_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTYPE, "%s(%d): type is %s, text() '%s' @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->text(),
           pc->orig_line, pc->orig_col);

   size_t    star_count = 0;
   size_t    word_count = 0;
   chunk_t   *ptrcnk    = nullptr;
   chunk_t   *tmp;
   chunk_t   *apo;
   chunk_t   *apc;
   chunk_t   *aft;
   bool      anon = false;
   c_token_t pt, ptp;

   // Scan backwards across the name, which can only be a word and single star
   chunk_t *varcnk = chunk_get_prev_ncnlni(pc);   // Issue #2279
   varcnk = chunk_get_prev_ssq(varcnk);

   if (varcnk != nullptr && !chunk_is_word(varcnk))
   {
      if (  language_is_set(LANG_OC)
         && chunk_is_str(varcnk, "^", 1)
         && chunk_is_paren_open(chunk_get_prev_ncnlni(varcnk)))   // Issue #2279
      {
         // anonymous ObjC block type -- RTYPE (^)(ARGS)
         anon = true;
      }
      else
      {
         LOG_FMT(LFTYPE, "%s(%d): not a word: text() '%s', type is %s, @ orig_line is %zu:, orig_col is %zu\n",
                 __func__, __LINE__, varcnk->text(), get_token_name(varcnk->type),
                 varcnk->orig_line, varcnk->orig_col);
         goto nogo_exit;
      }
   }
   apo = chunk_get_next_ncnl(pc);

   if (apo == nullptr)
   {
      return(false);
   }
   apc = chunk_skip_to_match(apo);

   if (  apc != nullptr
      && (  !chunk_is_paren_open(apo)
         || ((apc = chunk_skip_to_match(apo)) == nullptr)))
   {
      LOG_FMT(LFTYPE, "%s(%d): not followed by parens\n", __func__, __LINE__);
      goto nogo_exit;
   }
   aft = chunk_get_next_ncnl(apc);

   if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      pt = CT_FUNC_DEF;
   }
   else if (chunk_is_token(aft, CT_SEMICOLON) || chunk_is_token(aft, CT_ASSIGN))
   {
      pt = CT_FUNC_PROTO;
   }
   else
   {
      LOG_FMT(LFTYPE, "%s(%d): not followed by '{' or ';'\n", __func__, __LINE__);
      goto nogo_exit;
   }
   ptp = pc->flags.test(PCF_IN_TYPEDEF) ? CT_FUNC_TYPE : CT_FUNC_VAR;

   tmp = pc;

   while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
   {
      tmp = chunk_get_prev_ssq(tmp);

      LOG_FMT(LFTYPE, " -- type is %s, %s on orig_line %zu, orig_col is %zu",
              get_token_name(tmp->type), tmp->text(),
              tmp->orig_line, tmp->orig_col);

      if (  chunk_is_star(tmp)
         || chunk_is_token(tmp, CT_PTR_TYPE)
         || chunk_is_token(tmp, CT_CARET))
      {
         star_count++;
         ptrcnk = tmp;
         LOG_FMT(LFTYPE, " -- PTR_TYPE\n");
      }
      else if (  chunk_is_word(tmp)
              || chunk_is_token(tmp, CT_WORD)
              || chunk_is_token(tmp, CT_TYPE))
      {
         word_count++;
         LOG_FMT(LFTYPE, " -- TYPE(%s)\n", tmp->text());
      }
      else if (chunk_is_token(tmp, CT_DC_MEMBER))
      {
         word_count = 0;
         LOG_FMT(LFTYPE, " -- :: reset word_count\n");
      }
      else if (chunk_is_str(tmp, "(", 1))
      {
         LOG_FMT(LFTYPE, " -- open paren (break)\n");
         break;
      }
      else
      {
         LOG_FMT(LFTYPE, " --  unexpected token: type is %s, text() '%s', on orig_line %zu, orig_col %zu\n",
                 get_token_name(tmp->type), tmp->text(),
                 tmp->orig_line, tmp->orig_col);
         goto nogo_exit;
      }
   }

   // Fixes #issue 1577
   // Allow word count 2 incase of function pointer declaration.
   // Ex: bool (__stdcall* funcptr)(int, int);
   if (  star_count > 1
      || (word_count > 1 && !(word_count == 2 && ptp == CT_FUNC_VAR))
      || ((star_count + word_count) == 0))
   {
      LOG_FMT(LFTYPE, "%s(%d): bad counts word: %zu, star: %zu\n",
              __func__, __LINE__, word_count, star_count);
      goto nogo_exit;
   }

   // make sure what appears before the first open paren can be a return type
   if (!chunk_ends_type(chunk_get_prev_ncnlni(tmp)))   // Issue #2279
   {
      goto nogo_exit;
   }

   if (ptrcnk)
   {
      set_chunk_type(ptrcnk, CT_PTR_TYPE);
   }

   if (!anon)
   {
      if (pc->flags.test(PCF_IN_TYPEDEF))
      {
         set_chunk_type(varcnk, CT_TYPE);
      }
      else
      {
         set_chunk_type(varcnk, CT_FUNC_VAR);
         chunk_flags_set(varcnk, PCF_VAR_1ST_DEF);
      }
   }
   set_chunk_type(pc, CT_TPAREN_CLOSE);
   set_chunk_parent(pc, ptp);

   set_chunk_type(apo, CT_FPAREN_OPEN);
   set_chunk_parent(apo, pt);
   set_chunk_type(apc, CT_FPAREN_CLOSE);
   set_chunk_parent(apc, pt);
   fix_fcn_def_params(apo);

   if (chunk_is_semicolon(aft))
   {
      set_chunk_parent(aft, aft->flags.test(PCF_IN_TYPEDEF) ? CT_TYPEDEF : CT_FUNC_VAR);
   }
   else if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      flag_parens(aft, PCF_NONE, CT_NONE, pt, false);
   }
   // Step backwards to the previous open paren and mark everything a
   tmp = pc;

   while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
   {
      LOG_FMT(LFTYPE, " ++ type is %s, text() '%s', on orig_line %zu, orig_col %zu\n",
              get_token_name(tmp->type), tmp->text(),
              tmp->orig_line, tmp->orig_col);

      if (*tmp->str.c_str() == '(')
      {
         if (!pc->flags.test(PCF_IN_TYPEDEF))
         {
            chunk_flags_set(tmp, PCF_VAR_1ST_DEF);
         }
         set_chunk_type(tmp, CT_TPAREN_OPEN);
         set_chunk_parent(tmp, ptp);

         tmp = chunk_get_prev_ncnlni(tmp);   // Issue #2279

         if (  chunk_is_token(tmp, CT_FUNCTION)
            || chunk_is_token(tmp, CT_FUNC_CALL)
            || chunk_is_token(tmp, CT_FUNC_CALL_USER)
            || chunk_is_token(tmp, CT_FUNC_DEF)
            || chunk_is_token(tmp, CT_FUNC_PROTO))
         {
            set_chunk_type(tmp, CT_TYPE);
            chunk_flags_clr(tmp, PCF_VAR_1ST_DEF);
         }
         mark_function_return_type(varcnk, tmp, ptp);
         break;
      }
   }
   return(true);

nogo_exit:
   tmp = chunk_get_next_ncnl(pc);

   if (chunk_is_paren_open(tmp))
   {
      LOG_FMT(LFTYPE, "%s(%d): setting FUNC_CALL on orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col);
      flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
   }
   return(false);
} // mark_function_type


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


static bool is_ucase_str(const char *str, size_t len)
{
   while (len-- > 0)
   {
      if (unc_toupper(*str) != *str)
      {
         return(false);
      }
      str++;
   }
   return(true);
}


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


static void fix_casts(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc;
   chunk_t    *prev;
   chunk_t    *first;
   chunk_t    *after;
   chunk_t    *last = nullptr;
   chunk_t    *paren_close;
   const char *verb      = "likely";
   const char *detail    = "";
   size_t     count      = 0;
   int        word_count = 0;
   bool       nope;
   bool       doubtful_cast = false;


   LOG_FMT(LCASTS, "%s(%d): start->text() is '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, start->text(), start->orig_line, start->orig_col);

   prev = chunk_get_prev_ncnlni(start);   // Issue #2279

   if (prev == nullptr)
   {
      return;
   }

   if (chunk_is_token(prev, CT_PP_DEFINED))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - after defined\n",
              __func__, __LINE__);
      return;
   }

   if (chunk_is_token(prev, CT_ANGLE_CLOSE))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - after > (template)\n",
              __func__, __LINE__);
      return;
   }
   // Make sure there is only WORD, TYPE, and '*' or '^' before the close paren
   pc    = chunk_get_next_ncnl(start);
   first = pc;

   while (  pc != nullptr
         && (  chunk_is_type(pc)
            || chunk_is_token(pc, CT_WORD)
            || chunk_is_token(pc, CT_QUALIFIER)
            || chunk_is_token(pc, CT_DC_MEMBER)
            || chunk_is_token(pc, CT_PP)
            || chunk_is_token(pc, CT_STAR)
            || chunk_is_token(pc, CT_QUESTION)
            || chunk_is_token(pc, CT_CARET)
            || chunk_is_token(pc, CT_TSQUARE)
            || (  (  chunk_is_token(pc, CT_ANGLE_OPEN)
                  || chunk_is_token(pc, CT_ANGLE_CLOSE))
               && language_is_set(LANG_OC | LANG_JAVA))
            || (  (  chunk_is_token(pc, CT_QUESTION)
                  || chunk_is_token(pc, CT_COMMA)
                  || chunk_is_token(pc, CT_MEMBER))
               && language_is_set(LANG_JAVA))
            || chunk_is_token(pc, CT_AMP)))
   {
      LOG_FMT(LCASTS, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));

      if (chunk_is_token(pc, CT_WORD) || (chunk_is_token(last, CT_ANGLE_CLOSE) && chunk_is_token(pc, CT_DC_MEMBER)))
      {
         word_count++;
      }
      else if (chunk_is_token(pc, CT_DC_MEMBER) || chunk_is_token(pc, CT_MEMBER) || chunk_is_token(pc, CT_PP))
      {
         // might be negativ, such as with:
         // a = val + (CFoo::bar_t)7;
         word_count--;
      }
      last = pc;
      pc   = chunk_get_next_ncnl(pc);
      count++;
   }

   if (  pc == nullptr
      || pc->type != CT_PAREN_CLOSE
      || chunk_is_token(prev, CT_OC_CLASS))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast, hit type is %s\n",
              __func__, __LINE__, pc == nullptr ? "NULL" : get_token_name(pc->type));
      return;
   }

   if (word_count > 1)
   {
      LOG_FMT(LCASTS, "%s(%d):  -- too many words: %d\n",
              __func__, __LINE__, word_count);
      return;
   }
   paren_close = pc;

   // If last is a type or star/caret, we have a cast for sure
   if (  chunk_is_token(last, CT_STAR)
      || chunk_is_token(last, CT_CARET)
      || chunk_is_token(last, CT_PTR_TYPE)
      || chunk_is_token(last, CT_TYPE)
      || (chunk_is_token(last, CT_ANGLE_CLOSE) && language_is_set(LANG_OC | LANG_JAVA)))
   {
      verb = "for sure";
   }
   else if (count == 1)
   {
      /*
       * We are on a potential cast of the form "(word)".
       * We don't know if the word is a type. So lets guess based on some
       * simple rules:
       *  - if all caps, likely a type
       *  - if it ends in _t, likely a type
       *  - if it's objective-c and the type is id, likely valid
       */
      verb = "guessed";

      if (  (last->len() > 3)
         && (last->str[last->len() - 2] == '_')
         && (last->str[last->len() - 1] == 't'))
      {
         detail = " -- '_t'";
      }
      else if (is_ucase_str(last->text(), last->len()))
      {
         detail = " -- upper case";
      }
      else if (language_is_set(LANG_OC) && chunk_is_str(last, "id", 2))
      {
         detail = " -- Objective-C id";
      }
      else
      {
         // If we can't tell for sure whether this is a cast, decide against it
         detail        = " -- mixed case";
         doubtful_cast = true;
      }
      /*
       * If the next item is a * or &, the next item after that can't be a
       * number or string.
       *
       * If the next item is a +, the next item has to be a number.
       *
       * If the next item is a -, the next item can't be a string.
       *
       * For this to be a cast, the close paren must be followed by:
       *  - constant (number or string)
       *  - paren open
       *  - word
       *
       * Find the next non-open paren item.
       */
      pc    = chunk_get_next_ncnl(paren_close);
      after = pc;

      do
      {
         after = chunk_get_next_ncnl(after);
      } while (chunk_is_token(after, CT_PAREN_OPEN));

      if (after == nullptr)
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - hit NULL\n",
                 __func__, __LINE__);
         return;
      }
      nope = false;

      if (chunk_is_ptr_operator(pc))
      {
         // star (*) and address (&) are ambiguous
         if (  chunk_is_token(after, CT_NUMBER_FP)
            || chunk_is_token(after, CT_NUMBER)
            || chunk_is_token(after, CT_STRING)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (chunk_is_token(pc, CT_MINUS))
      {
         // (UINT8)-1 or (foo)-1 or (FOO)-'a'
         if (chunk_is_token(after, CT_STRING) || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (chunk_is_token(pc, CT_PLUS))
      {
         // (UINT8)+1 or (foo)+1
         if (  (after->type != CT_NUMBER && after->type != CT_NUMBER_FP)
            || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (  pc->type != CT_NUMBER_FP
              && pc->type != CT_NUMBER
              && pc->type != CT_WORD
              && pc->type != CT_THIS
              && pc->type != CT_TYPE
              && pc->type != CT_PAREN_OPEN
              && pc->type != CT_STRING
              && pc->type != CT_DECLTYPE
              && pc->type != CT_SIZEOF
              && get_chunk_parent_type(pc) != CT_SIZEOF
              && pc->type != CT_FUNC_CALL
              && pc->type != CT_FUNC_CALL_USER
              && pc->type != CT_FUNCTION
              && pc->type != CT_BRACE_OPEN
              && (!(  chunk_is_token(pc, CT_SQUARE_OPEN)
                   && language_is_set(LANG_OC))))
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by text() '%s', type is %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(pc->type));
         return;
      }

      if (nope)
      {
         LOG_FMT(LCASTS, "%s(%d):  -- not a cast - text() '%s' followed by type %s\n",
                 __func__, __LINE__, pc->text(), get_token_name(after->type));
         return;
      }
   }
   // if the 'cast' is followed by a semicolon, comma, bool or close parenthesis, it isn't
   pc = chunk_get_next_ncnl(paren_close);

   if (pc == nullptr)
   {
      return;
   }

   if (  chunk_is_semicolon(pc)
      || chunk_is_token(pc, CT_COMMA)
      || chunk_is_token(pc, CT_BOOL)               // Issue #2151
      || chunk_is_paren_close(pc))
   {
      LOG_FMT(LCASTS, "%s(%d):  -- not a cast - followed by type %s\n",
              __func__, __LINE__, get_token_name(pc->type));
      return;
   }
   set_chunk_parent(start, CT_C_CAST);
   set_chunk_parent(paren_close, CT_C_CAST);

   LOG_FMT(LCASTS, "%s(%d):  -- %s c-cast: (",
           __func__, __LINE__, verb);

   for (pc = first;
        pc != nullptr && pc != paren_close;
        pc = chunk_get_next_ncnl(pc))
   {
      set_chunk_parent(pc, CT_C_CAST);
      make_type(pc);
      LOG_FMT(LCASTS, " %s", pc->text());
   }

   LOG_FMT(LCASTS, " )%s\n", detail);

   // Mark the next item as an expression start
   pc = chunk_get_next_ncnl(paren_close);

   if (pc != nullptr)
   {
      chunk_flags_set(pc, PCF_EXPR_START);

      if (chunk_is_opening_brace(pc))
      {
         set_paren_parent(pc, get_chunk_parent_type(start));
      }
   }
} // fix_casts


static void fix_type_cast(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   pc = chunk_get_next_ncnl(start);

   if (pc == nullptr || pc->type != CT_ANGLE_OPEN)
   {
      return;
   }

   while (  ((pc = chunk_get_next_ncnl(pc)) != nullptr)
         && pc->level >= start->level)
   {
      if (pc->level == start->level && chunk_is_token(pc, CT_ANGLE_CLOSE))
      {
         pc = chunk_get_next_ncnl(pc);

         if (pc == nullptr)
         {
            return;
         }

         if (chunk_is_str(pc, "(", 1))
         {
            set_paren_parent(pc, CT_TYPE_CAST);
         }
         return;
      }
      make_type(pc);
   }
}


static void fix_enum_struct_union(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t     *next;
   chunk_t     *prev        = nullptr;
   pcf_flags_t flags        = PCF_VAR_1ST_DEF;
   auto const  in_fcn_paren = pc->flags & PCF_IN_FCN_DEF;

   // Make sure this wasn't a cast
   if (get_chunk_parent_type(pc) == CT_C_CAST)
   {
      return;
   }
   // the next item is either a type or open brace
   next = chunk_get_next_ncnl(pc);

   // the enum-key might be enum, enum class or enum struct (TODO)
   if (chunk_is_token(next, CT_ENUM_CLASS))
   {
      next = chunk_get_next_ncnl(next); // get the next one
   }

   // the next item is either a type, an attribute (TODO), an identifier, a colon or open brace
   if (chunk_is_token(next, CT_TYPE) || chunk_is_token(next, CT_WORD))
   {
      // i.e. "enum xyz : unsigned int { ... };"
      // i.e. "enum class xyz : unsigned int { ... };"
      // xyz is a type
      set_chunk_parent(next, pc->type);
      prev = next;                                               // save xyz
      next = chunk_get_next_ncnl(next);

      if (next == nullptr)
      {
         return;
      }
      set_chunk_parent(next, pc->type);
      auto const is_struct_or_class =
         (chunk_is_token(pc, CT_STRUCT) || chunk_is_token(pc, CT_CLASS));

      // next up is either a colon, open brace, or open parenthesis (pawn)
      if (language_is_set(LANG_PAWN) && chunk_is_token(next, CT_PAREN_OPEN))
      {
         next = set_paren_parent(next, CT_ENUM);
      }
      else if (chunk_is_token(next, CT_COLON))
      {
         if (chunk_is_token(pc, CT_ENUM))
         {
            // enum TYPE : INT_TYPE { ... };
            next = chunk_get_next_ncnl(next);

            if (next != nullptr)
            {
               make_type(next);
               next = chunk_get_next_ncnl(next);

               // enum TYPE : unsigned int { ... };
               if (chunk_is_token(next, CT_TYPE))
               {
                  // get the next part of the type
                  next = chunk_get_next_ncnl(next);
               }
            }
         }
         else if (is_struct_or_class)
         {
            next = skip_parent_types(next);
         }
      }
      else if (is_struct_or_class && chunk_is_token(next, CT_PAREN_OPEN))
      {
         // Fix #1267 structure attributes
         // struct __attribute__(align(x)) struct_name;
         // skip to matching parenclose and make next token as type.
         next = chunk_skip_to_match(next);
         next = chunk_get_next_ncnl(next);
         set_chunk_type(next, CT_TYPE);
         set_chunk_parent(next, pc->type);
      }

      if (chunk_is_token(next, CT_SEMICOLON)) // c++ forward declaration
      {
         set_chunk_parent(next, pc->type);
         flag_series(pc, prev, PCF_INCOMPLETE);
         return;
      }
   }

   if (chunk_is_token(next, CT_BRACE_OPEN))
   {
      auto const flag = [pc] {
         switch (pc->type)
         {
         case CT_ENUM:
            return(PCF_IN_ENUM);

         case CT_STRUCT:
            return(PCF_IN_STRUCT);

         case CT_CLASS:
            return(PCF_IN_CLASS);

         default:
            return(PCF_NONE);
         }
      }();

      flag_parens(next, flag, CT_NONE, CT_NONE, false);

      if (  chunk_is_token(pc, CT_UNION)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_CLASS))
      {
         mark_struct_union_body(next);
      }
      // Skip to the closing brace
      set_chunk_parent(next, pc->type);
      next   = chunk_get_next_type(next, CT_BRACE_CLOSE, pc->level);
      flags |= PCF_VAR_INLINE;

      if (next != nullptr)
      {
         set_chunk_parent(next, pc->type);
         next = chunk_get_next_ncnl(next);
      }
      prev = nullptr;
   }
   // reset var name parent type
   else if (next && prev)
   {
      set_chunk_parent(prev, CT_NONE);
   }

   if (next == nullptr || chunk_is_token(next, CT_PAREN_CLOSE))
   {
      return;
   }

   if (!chunk_is_semicolon(next))
   {
      // Pawn does not require a semicolon after an enum
      if (language_is_set(LANG_PAWN))
      {
         return;
      }

      /*
       * D does not require a semicolon after an enum, but we add one to make
       * other code happy.
       */
      if (language_is_set(LANG_D))
      {
         next = pawn_add_vsemi_after(chunk_get_prev_ncnlni(next));   // Issue #2279
      }
   }

   // We are either pointing to a ';' or a variable
   while (  next != nullptr
         && !chunk_is_semicolon(next)
         && next->type != CT_ASSIGN
         && !(in_fcn_paren ^ (next->flags & PCF_IN_FCN_DEF)).test_any())
   {
      if (next->level == pc->level)
      {
         if (chunk_is_token(next, CT_WORD))
         {
            chunk_flags_set(next, flags);
            flags &= ~PCF_VAR_1ST;   // clear the first flag for the next items
            LOG_FMT(LCASTS, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', set PCF_VAR_1ST\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
         }

         if (  chunk_is_token(next, CT_STAR)
            || (language_is_set(LANG_CPP) && chunk_is_token(next, CT_CARET)))
         {
            set_chunk_type(next, CT_PTR_TYPE);
         }

         // If we hit a comma in a function param, we are done
         if (  (chunk_is_token(next, CT_COMMA) || chunk_is_token(next, CT_FPAREN_CLOSE))
            && (next->flags.test_any(PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)))
         {
            return;
         }
      }
      next = chunk_get_next_ncnl(next);
   }

   if (  next != nullptr
      && chunk_is_token(next, CT_SEMICOLON))
   {
      set_chunk_parent(next, pc->type);
   }
} // fix_enum_struct_union


static void fix_typedef(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   LOG_FMT(LTYPEDEF, "%s(%d): typedef @ orig_line %zu, orig_col %zu\n",
           __func__, __LINE__, start->orig_line, start->orig_col);

   chunk_t *the_type = nullptr;
   chunk_t *last_op  = nullptr;

   /*
    * Mark everything in the typedef and scan for ")(", which makes it a
    * function type
    */
   for (chunk_t *next = chunk_get_next_ncnl(start, scope_e::PREPROC)
        ; next != nullptr && next->level >= start->level
        ; next = chunk_get_next_ncnl(next, scope_e::PREPROC))
   {
      chunk_flags_set(next, PCF_IN_TYPEDEF);

      if (start->level == next->level)
      {
         if (chunk_is_semicolon(next))
         {
            set_chunk_parent(next, CT_TYPEDEF);
            break;
         }

         if (chunk_is_token(next, CT_ATTRIBUTE))
         {
            break;
         }

         if (language_is_set(LANG_D) && chunk_is_token(next, CT_ASSIGN))
         {
            set_chunk_parent(next, CT_TYPEDEF);
            break;
         }
         make_type(next);

         if (chunk_is_token(next, CT_TYPE))
         {
            the_type = next;
         }
         chunk_flags_clr(next, PCF_VAR_1ST_DEF);

         if (*next->str.c_str() == '(')
         {
            last_op = next;
         }
      }
   }

   // avoid interpreting typedef NS_ENUM (NSInteger, MyEnum) as a function def
   if (  last_op != nullptr
      && !(language_is_set(LANG_OC) && get_chunk_parent_type(last_op) == CT_ENUM))
   {
      flag_parens(last_op, PCF_NONE, CT_FPAREN_OPEN, CT_TYPEDEF, false);
      fix_fcn_def_params(last_op);

      the_type = chunk_get_prev_ncnlni(last_op, scope_e::PREPROC);   // Issue #2279

      if (the_type == nullptr)
      {
         return;
      }
      chunk_t *open_paren = nullptr;

      if (chunk_is_paren_close(the_type))
      {
         open_paren = chunk_skip_to_match_rev(the_type);
         mark_function_type(the_type);
         the_type = chunk_get_prev_ncnlni(the_type, scope_e::PREPROC);   // Issue #2279

         if (the_type == nullptr)
         {
            return;
         }
      }
      else
      {
         // must be: "typedef <return type>func(params);"
         set_chunk_type(the_type, CT_FUNC_TYPE);
      }
      set_chunk_parent(the_type, CT_TYPEDEF);

      LOG_FMT(LTYPEDEF, "%s(%d): fcn typedef text() '%s', on orig_line %zu\n",
              __func__, __LINE__, the_type->text(), the_type->orig_line);

      // If we are aligning on the open parenthesis, grab that instead
      log_rule_B("align_typedef_func");

      if (open_paren != nullptr && options::align_typedef_func() == 1)
      {
         the_type = open_paren;
      }
      log_rule_B("align_typedef_func");

      if (options::align_typedef_func() != 0)
      {
         LOG_FMT(LTYPEDEF, "%s(%d):  -- align anchor on text() %s, @ orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, the_type->text(), the_type->orig_line, the_type->orig_col);
         chunk_flags_set(the_type, PCF_ANCHOR);
      }
      // already did everything we need to do
      return;
   }
   /*
    * Skip over enum/struct/union stuff, as we know it isn't a return type
    * for a function type
    */
   chunk_t *after = chunk_get_next_ncnl(start, scope_e::PREPROC);

   if (after == nullptr)
   {
      return;
   }

   if (  after->type != CT_ENUM
      && after->type != CT_STRUCT
      && after->type != CT_UNION)
   {
      if (the_type != nullptr)
      {
         // We have just a regular typedef
         LOG_FMT(LTYPEDEF, "%s(%d): regular typedef text() %s, on orig_line %zu\n",
                 __func__, __LINE__, the_type->text(), the_type->orig_line);
         chunk_flags_set(the_type, PCF_ANCHOR);
      }
      return;
   }
   // We have a struct/union/enum, next should be either a type or {
   chunk_t *next = chunk_get_next_ncnl(after, scope_e::PREPROC);

   if (next == nullptr)
   {
      return;
   }

   if (chunk_is_token(next, CT_TYPE))
   {
      next = chunk_get_next_ncnl(next, scope_e::PREPROC);

      if (next == nullptr)
      {
         return;
      }
   }

   if (chunk_is_token(next, CT_BRACE_OPEN))
   {
      // Skip to the closing brace
      chunk_t *br_c = chunk_get_next_type(next, CT_BRACE_CLOSE, next->level, scope_e::PREPROC);

      if (br_c != nullptr)
      {
         const c_token_t tag = after->type;
         set_chunk_parent(next, tag);
         set_chunk_parent(br_c, tag);

         if (tag == CT_ENUM)
         {
            flag_series(after, br_c, PCF_IN_ENUM);
         }
         else if (tag == CT_STRUCT)
         {
            flag_series(after, br_c, PCF_IN_STRUCT);
         }
      }
   }

   if (the_type != nullptr)
   {
      LOG_FMT(LTYPEDEF, "%s(%d): %s typedef text() %s, on orig_line %zu\n",
              __func__, __LINE__, get_token_name(after->type), the_type->text(),
              the_type->orig_line);
      chunk_flags_set(the_type, PCF_ANCHOR);
   }
} // fix_typedef


static void mark_variable_stack(ChunkStack &cs, log_sev_t sev)
{
   UNUSED(sev);
   LOG_FUNC_ENTRY();

   // throw out the last word and mark the rest
   chunk_t *var_name = cs.Pop_Back();

   if (var_name && var_name->prev->type == CT_DC_MEMBER)
   {
      cs.Push_Back(var_name);
   }

   if (var_name != nullptr)
   {
      LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu:\n",
              __func__, __LINE__, var_name->orig_line, var_name->orig_col);

      size_t  word_cnt = 0;
      chunk_t *word_type;

      while ((word_type = cs.Pop_Back()) != nullptr)
      {
         if (chunk_is_token(word_type, CT_WORD) || chunk_is_token(word_type, CT_TYPE))
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, word_type->text());
            set_chunk_type(word_type, CT_TYPE);
            chunk_flags_set(word_type, PCF_VAR_TYPE);
         }
         word_cnt++;
      }

      if (chunk_is_token(var_name, CT_WORD))
      {
         if (word_cnt > 0)
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as VAR\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, var_name->text());
            chunk_flags_set(var_name, PCF_VAR_DEF);
         }
         else
         {
            LOG_FMT(LFCNP, "%s(%d): parameter on orig_line %zu, orig_col %zu: <%s> as TYPE\n",
                    __func__, __LINE__, var_name->orig_line, var_name->orig_col, var_name->text());
            set_chunk_type(var_name, CT_TYPE);
            chunk_flags_set(var_name, PCF_VAR_TYPE);
         }
      }
   }
} // mark_variable_stack


static void fix_fcn_def_params(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return;
   }
   LOG_FMT(LFCNP, "%s(%d): text() '%s', type is %s, on orig_line %zu, level is %zu\n",
           __func__, __LINE__, start->text(), get_token_name(start->type), start->orig_line, start->level);

   while (start != nullptr && !chunk_is_paren_open(start))
   {
      start = chunk_get_next_ncnl(start);
   }

   if (start == nullptr)// Coverity CID 76003, 1100782
   {
      return;
   }
   // ensure start chunk holds a single '(' character
   assert((start->len() == 1) && (start->str[0] == '('));

   ChunkStack cs;
   size_t     level = start->level + 1;
   chunk_t    *pc   = start;

   while ((pc = chunk_get_next_ncnl(pc)) != nullptr)
   {
      if (  ((start->len() == 1) && (start->str[0] == ')'))
         || pc->level < level)
      {
         LOG_FMT(LFCNP, "%s(%d): bailed on text() '%s', on orig_line %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line);
         break;
      }
      LOG_FMT(LFCNP, "%s(%d): %s, text() '%s' on orig_line %zu, level %zu\n",
              __func__, __LINE__, (pc->level > level) ? "skipping" : "looking at",
              pc->text(), pc->orig_line, pc->level);

      if (pc->level > level)
      {
         continue;
      }

      if (chunk_is_star(pc) || chunk_is_msref(pc) || chunk_is_nullable(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
         cs.Push_Back(pc);
      }
      else if (  chunk_is_token(pc, CT_AMP)
              || (language_is_set(LANG_CPP) && chunk_is_str(pc, "&&", 2)))
      {
         set_chunk_type(pc, CT_BYREF);
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_TYPE_WRAP))
      {
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_TYPE))
      {
         cs.Push_Back(pc);
      }
      else if (chunk_is_token(pc, CT_COMMA) || chunk_is_token(pc, CT_ASSIGN))
      {
         mark_variable_stack(cs, LFCNP);

         if (chunk_is_token(pc, CT_ASSIGN))
         {
            // Mark assignment for default param spacing
            set_chunk_parent(pc, CT_FUNC_PROTO);
         }
      }
   }
   mark_variable_stack(cs, LFCNP);
} // fix_fcn_def_params


static chunk_t *skip_to_next_statement(chunk_t *pc)
{
   while (  pc != nullptr
         && !chunk_is_semicolon(pc)
         && pc->type != CT_BRACE_OPEN
         && pc->type != CT_BRACE_CLOSE)
   {
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


static chunk_t *fix_var_def(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc = start;
   chunk_t    *end;
   chunk_t    *tmp_pc;
   ChunkStack cs;
   int        idx;
   int        ref_idx;

   LOG_FMT(LFVD, "%s(%d): start at pc->orig_line is %zu, pc->orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);

   // Scan for words and types and stars oh my!
   while (  chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_WORD)
         || chunk_is_token(pc, CT_QUALIFIER)
         || chunk_is_token(pc, CT_TYPENAME)
         || chunk_is_token(pc, CT_DC_MEMBER)
         || chunk_is_token(pc, CT_MEMBER)
         || chunk_is_ptr_operator(pc))
   {
      LOG_FMT(LFVD, "%s(%d):   1:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));
      cs.Push_Back(pc);
      pc = chunk_get_next_ncnl(pc);

      if (pc == nullptr)
      {
         LOG_FMT(LFVD, "%s(%d): pc is nullptr\n", __func__, __LINE__);
         return(nullptr);
      }
      LOG_FMT(LFVD, "%s(%d):   2:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      // Skip templates and attributes
      pc = skip_template_next(pc);

      if (pc == nullptr)
      {
         LOG_FMT(LFVD, "%s(%d): pc is nullptr\n", __func__, __LINE__);
         return(nullptr);
      }
      LOG_FMT(LFVD, "%s(%d):   3:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      pc = skip_attribute_next(pc);

      if (pc == nullptr)
      {
         LOG_FMT(LFVD, "%s(%d): pc is nullptr\n", __func__, __LINE__);
         return(nullptr);
      }
      LOG_FMT(LFVD, "%s(%d):   4:pc->text() '%s', type is %s\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type));

      if (language_is_set(LANG_JAVA))
      {
         pc = skip_tsquare_next(pc);
         LOG_FMT(LFVD, "%s(%d):   5:pc->text() '%s', type is %s\n", __func__, __LINE__, pc->text(), get_token_name(pc->type));
      }
   }
   end = pc;

   if (end == nullptr)
   {
      LOG_FMT(LFVD, "%s(%d): end is nullptr\n", __func__, __LINE__);
      return(nullptr);
   }
   LOG_FMT(LFVD, "\n%s(%d): end->type is %s\n", __func__, __LINE__, get_token_name(end->type));

   if (  cs.Len() == 1
      && chunk_is_token(end, CT_BRACE_OPEN)
      && get_chunk_parent_type(end) == CT_BRACED_INIT_LIST)
   {
      set_chunk_type(cs.Get(0)->m_pc, CT_TYPE);
   }

   // Function defs are handled elsewhere
   if (  (cs.Len() <= 1)
      || chunk_is_token(end, CT_FUNC_DEF)
      || chunk_is_token(end, CT_FUNC_PROTO)
      || chunk_is_token(end, CT_FUNC_CLASS_DEF)
      || chunk_is_token(end, CT_FUNC_CLASS_PROTO)
      || chunk_is_token(end, CT_OPERATOR))
   {
      return(skip_to_next_statement(end));
   }
   // ref_idx points to the alignable part of the var def
   ref_idx = cs.Len() - 1;

   // Check for the '::' stuff: "char *Engine::name"
   if (  (cs.Len() >= 3)
      && (  (cs.Get(cs.Len() - 2)->m_pc->type == CT_MEMBER)
         || (cs.Get(cs.Len() - 2)->m_pc->type == CT_DC_MEMBER)))
   {
      idx = cs.Len() - 2;

      while (idx > 0)
      {
         tmp_pc = cs.Get(idx)->m_pc;

         if (tmp_pc->type != CT_DC_MEMBER && tmp_pc->type != CT_MEMBER)
         {
            break;
         }

         if (idx == 0)
         {
            fprintf(stderr, "%s(%d): idx is ZERO, cannot be decremented, at line %zu, column %zu\n",
                    __func__, __LINE__, tmp_pc->orig_line, tmp_pc->orig_col);
            log_flush(true);
            exit(EX_SOFTWARE);
         }
         idx--;
         tmp_pc = cs.Get(idx)->m_pc;

         if (tmp_pc->type != CT_WORD && tmp_pc->type != CT_TYPE)
         {
            break;
         }
         make_type(tmp_pc);
         idx--;
      }
      ref_idx = idx + 1;
   }
   tmp_pc = cs.Get(ref_idx)->m_pc;
   LOG_FMT(LFVD, " ref_idx(%d) => %s\n", ref_idx, tmp_pc->text());

   // No type part found!
   if (ref_idx <= 0)
   {
      return(skip_to_next_statement(end));
   }
   LOG_FMT(LFVD2, "%s(%d): orig_line is %zu, TYPE : ", __func__, __LINE__, start->orig_line);

   for (size_t idxForCs = 0; idxForCs < cs.Len() - 1; idxForCs++)
   {
      tmp_pc = cs.Get(idxForCs)->m_pc;
      make_type(tmp_pc);
      chunk_flags_set(tmp_pc, PCF_VAR_TYPE);
      LOG_FMT(LFVD2, " text() is '%s', type is %s", tmp_pc->text(), get_token_name(tmp_pc->type));
   }

   LOG_FMT(LFVD2, "\n");

   // OK we have two or more items, mark types up to the end.
   LOG_FMT(LFVD, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);
   mark_variable_definition(cs.Get(cs.Len() - 1)->m_pc);

   if (chunk_is_token(end, CT_COMMA))
   {
      return(chunk_get_next_ncnl(end));
   }
   return(skip_to_next_statement(end));
} // fix_var_def


static chunk_t *skip_expression(chunk_t *start)
{
   chunk_t *pc = start;

   while (pc != nullptr && pc->level >= start->level)
   {
      if (  pc->level == start->level
         && (chunk_is_semicolon(pc) || chunk_is_token(pc, CT_COMMA)))
      {
         return(pc);
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


bool go_on(chunk_t *pc, chunk_t *start)
{
   if (pc == nullptr || pc->level != start->level)
   {
      return(false);
   }

   if (pc->flags.test(PCF_IN_FOR))
   {
      return((!chunk_is_semicolon(pc)) && (!(chunk_is_token(pc, CT_COLON))));
   }
   return(!chunk_is_semicolon(pc));
} // go_on


static chunk_t *mark_variable_definition(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   if (start == nullptr)
   {
      return(nullptr);
   }
   chunk_t     *pc   = start;
   pcf_flags_t flags = PCF_VAR_1ST_DEF;

   LOG_FMT(LVARDEF, "%s(%d): orig_line %zu, orig_col %zu, text() '%s', type is %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
           get_token_name(pc->type));

   pc = start;

   // issue #596
   while (go_on(pc, start))
   {
      if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_FUNC_CTOR_VAR))
      {
         auto const orig_flags = pc->flags;

         if (!pc->flags.test(PCF_IN_ENUM))
         {
            chunk_flags_set(pc, flags);
         }
         flags &= ~PCF_VAR_1ST;
         LOG_FMT(LVARDEF, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', set PCF_VAR_1ST\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

         LOG_FMT(LVARDEF,
                 "%s(%d): orig_line is %zu, marked text() '%s'[%s] "
                 "in orig_col %zu, flags: %s -> %s\n",
                 __func__, __LINE__, pc->orig_line, pc->text(),
                 get_token_name(pc->type), pc->orig_col,
                 pcf_flags_str(orig_flags).c_str(),
                 pcf_flags_str(pc->flags).c_str());
      }
      else if (chunk_is_star(pc) || chunk_is_msref(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_addr(pc))
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else if (chunk_is_token(pc, CT_SQUARE_OPEN) || chunk_is_token(pc, CT_ASSIGN))
      {
         pc = skip_expression(pc);
         continue;
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
} // mark_variable_definition


static bool can_be_full_param(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFPARAM, "%s:", __func__);

   int     word_count = 0;
   int     type_count = 0;
   chunk_t *pc;

   for (pc = start;
        pc != nullptr && pc != end;
        pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
   {
      LOG_FMT(LFPARAM, " [%s]", pc->text());

      if (  chunk_is_token(pc, CT_QUALIFIER)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_ENUM)
         || chunk_is_token(pc, CT_UNION)
         || chunk_is_token(pc, CT_TYPENAME))
      {
         LOG_FMT(LFPARAM, " <== %s! (yes)\n", get_token_name(pc->type));
         return(true);
      }

      if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_TYPE))
      {
         ++word_count;

         if (chunk_is_token(pc, CT_TYPE))
         {
            ++type_count;
         }
      }
      else if (chunk_is_token(pc, CT_MEMBER) || chunk_is_token(pc, CT_DC_MEMBER))
      {
         if (word_count > 0)
         {
            --word_count;
         }
      }
      else if (pc != start && chunk_is_ptr_operator(pc))
      {
         // chunk is OK
      }
      else if (chunk_is_token(pc, CT_ASSIGN))
      {
         // chunk is OK (default values)
         break;
      }
      else if (chunk_is_token(pc, CT_ANGLE_OPEN))
      {
         LOG_FMT(LFPARAM, " <== template\n");
         return(true);
      }
      else if (chunk_is_token(pc, CT_ELLIPSIS))
      {
         LOG_FMT(LFPARAM, " <== elipses\n");
         return(true);
      }
      else if (word_count == 0 && chunk_is_token(pc, CT_PAREN_OPEN))
      {
         // Check for old-school func proto param '(type)'
         chunk_t *tmp1 = chunk_skip_to_match(pc, scope_e::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }
         chunk_t *tmp2 = chunk_get_next_ncnl(tmp1, scope_e::PREPROC);

         if (tmp2 == nullptr)
         {
            return(false);
         }

         if (chunk_is_token(tmp2, CT_COMMA) || chunk_is_paren_close(tmp2))
         {
            do
            {
               pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);

               if (pc == nullptr)
               {
                  return(false);
               }
               LOG_FMT(LFPARAM, " [%s]", pc->text());
            } while (pc != tmp1);

            // reset some vars to allow [] after parens
            word_count = 1;
            type_count = 1;
         }
         else
         {
            LOG_FMT(LFPARAM, " <== [%s] not fcn type!\n", get_token_name(pc->type));
            return(false);
         }
      }
      else if (  (word_count == 1 || (word_count == type_count))
              && chunk_is_token(pc, CT_PAREN_OPEN))
      {
         // Check for func proto param 'void (*name)' or 'void (*name)(params)' or 'void (^name)(params)'
         // <name> can be optional
         chunk_t *tmp1 = chunk_get_next_ncnl(pc, scope_e::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }
         chunk_t *tmp2 = chunk_get_next_ncnl(tmp1, scope_e::PREPROC);

         if (tmp2 == nullptr)
         {
            return(false);
         }
         chunk_t *tmp3 = (chunk_is_str(tmp2, ")", 1)) ? tmp2 : chunk_get_next_ncnl(tmp2, scope_e::PREPROC);

         if (tmp3 == nullptr)
         {
            return(false);
         }

         if (  !chunk_is_str(tmp3, ")", 1)
            || !(chunk_is_str(tmp1, "*", 1) || chunk_is_str(tmp1, "^", 1)) // Issue #2656
            || !(tmp2->type == CT_WORD || chunk_is_str(tmp2, ")", 1)))
         {
            LOG_FMT(LFPARAM, " <== [%s] not fcn type!\n", get_token_name(pc->type));
            return(false);
         }
         LOG_FMT(LFPARAM, " <skip fcn type>");
         tmp1 = chunk_get_next_ncnl(tmp3, scope_e::PREPROC);

         if (tmp1 == nullptr)
         {
            return(false);
         }

         if (chunk_is_str(tmp1, "(", 1))
         {
            tmp3 = chunk_skip_to_match(tmp1, scope_e::PREPROC);
         }
         pc = tmp3;

         // reset some vars to allow [] after parens
         word_count = 1;
         type_count = 1;
      }
      else if (chunk_is_token(pc, CT_TSQUARE))
      {
         // ignore it
      }
      else if (word_count == 1 && chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         // skip over any array stuff
         pc = chunk_skip_to_match(pc, scope_e::PREPROC);
      }
      else if (word_count == 2 && chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         // Bug #671: is it such as: bool foo[FOO_MAX]
         pc = chunk_skip_to_match(pc, scope_e::PREPROC);
      }
      else if (  word_count == 1
              && language_is_set(LANG_CPP)
              && chunk_is_str(pc, "&&", 2))
      {
         // ignore possible 'move' operator
      }
      else
      {
         LOG_FMT(LFPARAM, " <== [%s] no way! tc=%d wc=%d\n",
                 get_token_name(pc->type), type_count, word_count);
         return(false);
      }
   }

   chunk_t *last = chunk_get_prev_ncnlni(pc);   // Issue #2279

   if (chunk_is_ptr_operator(last))
   {
      LOG_FMT(LFPARAM, " <== [%s] sure!\n", get_token_name(pc->type));
      return(true);
   }

   if (word_count < 2 && type_count < 1 && start->brace_level > 0)
   {
      LOG_FMT(LFPARAM, " !MVP!");
      // Oh, joy, we are in Most Vexing Parse territory
      auto const brace =
         chunk_get_prev_type(start, CT_BRACE_OPEN, start->brace_level - 1);

      if (brace)
      {
         LOG_FMT(LFPARAM, " (matching %s brace at %zu:%zu)",
                 get_token_name(get_chunk_parent_type(brace)),
                 brace->orig_line, brace->orig_col);
      }

      if (  brace
         && (  get_chunk_parent_type(brace) == CT_CLASS
            || get_chunk_parent_type(brace) == CT_STRUCT))
      {
         // A Most Vexing Parse variable declaration cannot occur in the body
         // of a struct/class, so we probably have a function prototype
         LOG_FMT(LFPARAM, " <== [%s] Likely!\n",
                 (pc == nullptr ? "nullptr" : get_token_name(pc->type)));
         return(true);
      }
   }
   bool ret = (  word_count >= 2
              || (word_count == 1 && type_count == 1));

   LOG_FMT(LFPARAM, " <== [%s] %s!\n",
           (pc == nullptr ? "nullptr" : get_token_name(pc->type)),
           ret ? "Yup" : "Unlikely");
   return(ret);
} // can_be_full_param


static void mark_function(chunk_t *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return;
   }
   LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());
   chunk_t *prev = chunk_get_prev_ncnlni(pc);   // Issue #2279
   chunk_t *next = chunk_get_next_ncnlnp(pc);

   if (next == nullptr)
   {
      return;
   }
   chunk_t *tmp;
   chunk_t *semi = nullptr;
   chunk_t *paren_open;
   chunk_t *paren_close;

   // Find out what is before the operator
   if (get_chunk_parent_type(pc) == CT_OPERATOR)
   {
      chunk_t *pc_op = chunk_get_prev_type(pc, CT_OPERATOR, pc->level);

      if (pc_op != nullptr && pc_op->flags.test(PCF_EXPR_START))
      {
         set_chunk_type(pc, CT_FUNC_CALL);
      }

      if (language_is_set(LANG_CPP))
      {
         tmp = pc;

         while ((tmp = chunk_get_prev_ncnlni(tmp)) != nullptr)   // Issue #2279
         {
            if (  chunk_is_token(tmp, CT_BRACE_CLOSE)
               || chunk_is_token(tmp, CT_BRACE_OPEN)   // Issue 575
               || chunk_is_token(tmp, CT_SEMICOLON))
            {
               break;
            }

            if (chunk_is_paren_open(tmp))
            {
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }

            if (chunk_is_token(tmp, CT_ASSIGN))
            {
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }

            if (chunk_is_token(tmp, CT_TEMPLATE))
            {
               set_chunk_type(pc, CT_FUNC_DEF);
               break;
            }

            if (chunk_is_token(tmp, CT_BRACE_OPEN))
            {
               if (get_chunk_parent_type(tmp) == CT_FUNC_DEF)
               {
                  set_chunk_type(pc, CT_FUNC_CALL);
               }

               if (  get_chunk_parent_type(tmp) == CT_CLASS
                  || get_chunk_parent_type(tmp) == CT_STRUCT)
               {
                  set_chunk_type(pc, CT_FUNC_DEF);
               }
               break;
            }
         }

         if (tmp != nullptr && pc->type != CT_FUNC_CALL)
         {
            // Mark the return type
            while ((tmp = chunk_get_next_ncnl(tmp)) != pc && tmp != nullptr)
            {
               make_type(tmp); // Mark the return type
            }
         }
      }
   }

   if (chunk_is_ptr_operator(next))
   {
      next = chunk_get_next_ncnlnp(next);

      if (next == nullptr)
      {
         return;
      }
   }
   LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s, type is %s, parent_type is %s\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(),
           get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)));
   LOG_FMT(LFCN, "   level is %zu, brace_level is %zu, next->text() '%s', next->type is %s, next->level is %zu\n",
           pc->level, pc->brace_level,
           next->text(), get_token_name(next->type), next->level);

   if (pc->flags.test(PCF_IN_CONST_ARGS))
   {
      set_chunk_type(pc, CT_FUNC_CTOR_VAR);
      LOG_FMT(LFCN, "%s(%d):   1) Marked [%s] as FUNC_CTOR_VAR on line %zu col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      next = skip_template_next(next);

      if (next == nullptr)
      {
         return;
      }
      flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, pc->type, true);
      return;
   }
   // Skip over any template and attribute madness
   next = skip_template_next(next);

   if (next == nullptr)
   {
      return;
   }
   next = skip_attribute_next(next);

   if (next == nullptr)
   {
      return;
   }
   // Find the open and close parenthesis
   paren_open  = chunk_get_next_str(pc, "(", 1, pc->level);
   paren_close = chunk_get_next_str(paren_open, ")", 1, pc->level);

   if (paren_open == nullptr || paren_close == nullptr)
   {
      LOG_FMT(LFCN, "%s(%d): No parens found for [%s] on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      return;
   }
   /*
    * This part detects either chained function calls or a function ptr definition.
    * MYTYPE (*func)(void);
    * mWriter( "class Clst_"c )( somestr.getText() )( " : Cluster {"c ).newline;
    *
    * For it to be a function variable def, there must be a '*' followed by a
    * single word.
    *
    * Otherwise, it must be chained function calls.
    */
   tmp = chunk_get_next_ncnl(paren_close);

   if (tmp != nullptr && chunk_is_str(tmp, "(", 1))
   {
      chunk_t *tmp1;
      chunk_t *tmp2;
      chunk_t *tmp3;

      // skip over any leading class/namespace in: "T(F::*A)();"
      tmp1 = chunk_get_next_ncnl(next);

      while (tmp1 != nullptr)
      {
         tmp2 = chunk_get_next_ncnl(tmp1);

         if (!chunk_is_word(tmp1) || !chunk_is_token(tmp2, CT_DC_MEMBER))
         {
            break;
         }
         tmp1 = chunk_get_next_ncnl(tmp2);
      }
      tmp2 = chunk_get_next_ncnl(tmp1);

      if (chunk_is_str(tmp2, ")", 1))
      {
         tmp3 = tmp2;
         tmp2 = nullptr;
      }
      else
      {
         tmp3 = chunk_get_next_ncnl(tmp2);
      }
      tmp3 = chunk_get_next_ssq(tmp3);

      if (  chunk_is_str(tmp3, ")", 1)
         && (  chunk_is_star(tmp1)
            || chunk_is_msref(tmp1)
            || (language_is_set(LANG_OC) && chunk_is_token(tmp1, CT_CARET)))
         && (tmp2 == nullptr || chunk_is_token(tmp2, CT_WORD)))
      {
         if (tmp2)
         {
            LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, function variable '%s', changing '%s' into a type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, tmp2->text(), pc->text());
            set_chunk_type(tmp2, CT_FUNC_VAR);
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_VAR, false);

            LOG_FMT(LFCN, "%s(%d): paren open @ orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, paren_open->orig_line, paren_open->orig_col);
         }
         else
         {
            LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, function type, changing '%s' into a type\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

            if (tmp2)
            {
               set_chunk_type(tmp2, CT_FUNC_TYPE);
            }
            flag_parens(paren_open, PCF_NONE, CT_PAREN_OPEN, CT_FUNC_TYPE, false);
         }
         set_chunk_type(pc, CT_TYPE);
         set_chunk_type(tmp1, CT_PTR_TYPE);
         chunk_flags_clr(pc, PCF_VAR_1ST_DEF);

         if (tmp2 != nullptr)
         {
            chunk_flags_set(tmp2, PCF_VAR_1ST_DEF);
         }
         flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_PROTO, false);
         fix_fcn_def_params(tmp);
         return;
      }
      LOG_FMT(LFCN, "%s(%d): chained function calls? text() is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   }

   // Assume it is a function call if not already labeled
   if (chunk_is_token(pc, CT_FUNCTION))
   {
      LOG_FMT(LFCN, "%s(%d): examine: text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
      // look for an assigment. Issue #575
      chunk_t *temp = chunk_get_next_type(pc, CT_ASSIGN, pc->level);

      if (temp != nullptr)
      {
         LOG_FMT(LFCN, "%s(%d): assigment found, orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, temp->orig_line, temp->orig_col, temp->text());
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      else
      {
         set_chunk_type(pc, (get_chunk_parent_type(pc) == CT_OPERATOR) ? CT_FUNC_DEF : CT_FUNC_CALL);
      }
   }
   LOG_FMT(LFCN, "%s(%d): Check for C++ function def, text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
           __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));

   if (prev != nullptr)
   {
      LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));
   }

   // Check for C++ function def
   if (  chunk_is_token(pc, CT_FUNC_CLASS_DEF)
      || (  prev != nullptr
         && (  chunk_is_token(prev, CT_INV)
            || chunk_is_token(prev, CT_DC_MEMBER))))
   {
      chunk_t *destr = nullptr;

      if (chunk_is_token(prev, CT_INV))
      {
         // TODO: do we care that this is the destructor?
         set_chunk_type(prev, CT_DESTRUCTOR);
         set_chunk_type(pc, CT_FUNC_CLASS_DEF);

         set_chunk_parent(pc, CT_DESTRUCTOR);

         destr = prev;
         // Point to the item previous to the class name
         prev = chunk_get_prev_ncnlnp(prev);
      }

      if (chunk_is_token(prev, CT_DC_MEMBER))
      {
         prev = chunk_get_prev_ncnlnp(prev);
         LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                 get_token_name(prev->type));
         prev = skip_template_prev(prev);
         LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                 get_token_name(prev->type));
         prev = skip_attribute_prev(prev);
         LOG_FMT(LFCN, "%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                 get_token_name(prev->type));

         if (chunk_is_token(prev, CT_WORD) || chunk_is_token(prev, CT_TYPE))
         {
            if (pc->str.equals(prev->str))
            {
               LOG_FMT(LFCN, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col,
                       get_token_name(prev->type));
               set_chunk_type(pc, CT_FUNC_CLASS_DEF);
               LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu - FOUND %sSTRUCTOR for '%s', type is %s\n",
                       __func__, __LINE__,
                       prev->orig_line, prev->orig_col,
                       (destr != nullptr) ? "DE" : "CON",
                       prev->text(), get_token_name(prev->type));

               mark_cpp_constructor(pc);
               return;
            }
            // Point to the item previous to the class name
            prev = chunk_get_prev_ncnlnp(prev);
         }
      }
   }

   /*
    * Determine if this is a function call or a function def/proto
    * We check for level==1 to allow the case that a function prototype is
    * wrapped in a macro: "MACRO(void foo(void));"
    */
   if (  chunk_is_token(pc, CT_FUNC_CALL)
      && (  pc->level == pc->brace_level
         || pc->level == 1)
      && !pc->flags.test(PCF_IN_ARRAY_ASSIGN))
   {
      bool isa_def  = false;
      bool hit_star = false;
      LOG_FMT(LFCN, "%s(%d): pc->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col,
              get_token_name(pc->type));

      if (prev == nullptr)
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev is NULL\n",
                 __func__, __LINE__);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): Checking func call: prev->text() '%s', prev->type is %s\n",
                 __func__, __LINE__, prev->text(), get_token_name(prev->type));
      }
      // if (!chunk_ends_type(prev))
      // {
      //    goto bad_ret_type;
      // }

      /*
       * REVISIT:
       * a function def can only occur at brace level, but not inside an
       * assignment, structure, enum, or union.
       * The close paren must be followed by an open brace, with an optional
       * qualifier (const) in between.
       * There can be all sorts of template stuff and/or '[]' in the type.
       * This hack mostly checks that.
       *
       * Examples:
       * foo->bar(maid);                   -- fcn call
       * FOO * bar();                      -- fcn proto or class variable
       * FOO foo();                        -- fcn proto or class variable
       * FOO foo(1);                       -- class variable
       * a = FOO * bar();                  -- fcn call
       * a.y = foo() * bar();              -- fcn call
       * static const char * const fizz(); -- fcn def
       */
      while (prev != nullptr)
      {
         LOG_FMT(LFCN, "%s(%d): next step with: prev->orig_line is %zu, orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());

         if (get_chunk_parent_type(pc) == CT_FIXED)
         {
            isa_def = true;
         }

         if (prev->flags.test(PCF_IN_PREPROC))
         {
            prev = chunk_get_prev_ncnlnp(prev);
            continue;
         }

         // Some code slips an attribute between the type and function
         if (  chunk_is_token(prev, CT_FPAREN_CLOSE)
            && get_chunk_parent_type(prev) == CT_ATTRIBUTE)
         {
            prev = skip_attribute_prev(prev);
            continue;
         }

         // skip const(TYPE)
         if (chunk_is_token(prev, CT_PAREN_CLOSE) && get_chunk_parent_type(prev) == CT_D_CAST)
         {
            LOG_FMT(LFCN, "%s(%d): --> For sure a prototype or definition\n",
                    __func__, __LINE__);
            isa_def = true;
            break;
         }

         if (get_chunk_parent_type(prev) == CT_DECLSPEC)  // Issue 1289
         {
            prev = chunk_skip_to_match_rev(prev);
            prev = chunk_get_prev(prev);

            if (chunk_is_token(prev, CT_DECLSPEC))
            {
               prev = chunk_get_prev(prev);
            }
         }

         // if it was determined that this could be a function definition
         // but one of the preceding tokens is a CT_MEMBER than this is not a
         // fcn def, issue #1466
         if (isa_def && chunk_is_token(prev, CT_MEMBER))
         {
            isa_def = false;
         }

         // get first chunk before: A::B::pc | this.B.pc | this->B->pc
         if (chunk_is_token(prev, CT_DC_MEMBER) || chunk_is_token(prev, CT_MEMBER))
         {
            while (  chunk_is_token(prev, CT_DC_MEMBER)
                  || chunk_is_token(prev, CT_MEMBER))
            {
               prev = chunk_get_prev_ncnlnp(prev);

               if (  prev == nullptr
                  || (  prev->type != CT_WORD
                     && prev->type != CT_TYPE
                     && prev->type != CT_THIS))
               {
                  LOG_FMT(LFCN, "%s(%d): --? skipped MEMBER and landed on %s\n",
                          __func__, __LINE__, (prev == nullptr) ? "<null>" : get_token_name(prev->type));
                  break;
               }
               LOG_FMT(LFCN, "%s(%d): <skip> '%s'\n",
                       __func__, __LINE__, prev->text());

               // Issue #1112
               // clarification: this will skip the CT_WORD, CT_TYPE or CT_THIS landing on either
               // another CT_DC_MEMBER or CT_MEMBER or a token that indicates the context of the
               // token in question; therefore, exit loop when not a CT_DC_MEMBER or CT_MEMBER
               prev = chunk_get_prev_ncnlnp(prev);

               if (prev == nullptr)
               {
                  LOG_FMT(LFCN, "%s(%d): prev is nullptr\n",
                          __func__, __LINE__);
               }
               else
               {
                  LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s'\n",
                          __func__, __LINE__, prev->orig_line, prev->orig_col, prev->text());
               }
            }

            if (prev == nullptr)
            {
               break;
            }
         }

         // If we are on a TYPE or WORD, then this could be a proto or def
         if (chunk_is_token(prev, CT_TYPE) || chunk_is_token(prev, CT_WORD))
         {
            if (!hit_star)
            {
               LOG_FMT(LFCN, "%s(%d):   --> For sure a prototype or definition\n",
                       __func__, __LINE__);
               isa_def = true;
               break;
            }
            chunk_t *prev_prev = chunk_get_prev_ncnlnp(prev);

            if (!chunk_is_token(prev_prev, CT_QUESTION))               // Issue #1753
            {
               LOG_FMT(LFCN, "%s(%d):   --> maybe a proto/def\n",
                       __func__, __LINE__);

               LOG_FMT(LFCN, "%s(%d): prev is '%s', orig_line is %zu, orig_col is %zu, type is %s, parent_type is %s\n",
                       __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col,
                       get_token_name(prev->type), get_token_name(get_chunk_parent_type(prev)));
               log_pcf_flags(LFCN, pc->flags);
               isa_def = true;
            }
         }

         if (chunk_is_ptr_operator(prev))
         {
            hit_star = true;
         }

         if (  prev->type != CT_OPERATOR
            && prev->type != CT_TSQUARE
            && prev->type != CT_ANGLE_CLOSE
            && prev->type != CT_QUALIFIER
            && prev->type != CT_TYPE
            && prev->type != CT_WORD
            && !chunk_is_ptr_operator(prev))
         {
            LOG_FMT(LFCN, "%s(%d):  --> Stopping on prev is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                    __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));

            // certain tokens are unlikely to precede a prototype or definition
            if (  chunk_is_token(prev, CT_ARITH)
               || chunk_is_token(prev, CT_ASSIGN)
               || chunk_is_token(prev, CT_COMMA)
               || (chunk_is_token(prev, CT_STRING) && get_chunk_parent_type(prev) != CT_EXTERN)  // fixes issue 1259
               || chunk_is_token(prev, CT_STRING_MULTI)
               || chunk_is_token(prev, CT_NUMBER)
               || chunk_is_token(prev, CT_NUMBER_FP)
               || chunk_is_token(prev, CT_FPAREN_OPEN)) // issue #1464
            {
               isa_def = false;
            }
            break;
         }

         // Skip over template and attribute stuff
         if (chunk_is_token(prev, CT_ANGLE_CLOSE))
         {
            prev = skip_template_prev(prev);
         }
         else
         {
            prev = chunk_get_prev_ncnlnp(prev);
         }
      }
      //LOG_FMT(LFCN, " -- stopped on %s [%s]\n",
      //        prev->text(), get_token_name(prev->type));

      // Fixes issue #1634
      if (chunk_is_paren_close(prev))
      {
         chunk_t *preproc = chunk_get_next_ncnl(prev);

         if (chunk_is_token(preproc, CT_PREPROC))
         {
            size_t pp_level = preproc->pp_level;

            if (chunk_is_token(chunk_get_next_ncnl(preproc), CT_PP_ELSE))
            {
               do
               {
                  preproc = chunk_get_prev_ncnlni(preproc);      // Issue #2279

                  if (chunk_is_token(preproc, CT_PP_IF))
                  {
                     preproc = chunk_get_prev_ncnlni(preproc);   // Issue #2279

                     if (preproc->pp_level == pp_level)
                     {
                        prev = chunk_get_prev_ncnlnp(preproc);
                        break;
                     }
                  }
               } while (preproc != nullptr);
            }
         }
      }

      if (  isa_def
         && prev != nullptr
         && (  (chunk_is_paren_close(prev) && get_chunk_parent_type(prev) != CT_D_CAST)
            || prev->type == CT_ASSIGN
            || prev->type == CT_RETURN))
      {
         LOG_FMT(LFCN, "%s(%d): -- overriding DEF due to prev is '%s', type is %s\n",
                 __func__, __LINE__, prev->text(), get_token_name(prev->type));
         isa_def = false;
      }

      // Fixes issue #1266, identification of a tuple return type in CS.
      if (  !isa_def
         && chunk_is_token(prev, CT_PAREN_CLOSE)
         && chunk_get_next_ncnl(prev) == pc)
      {
         tmp = chunk_skip_to_match_rev(prev);

         while (  tmp != nullptr                     // Issue #2315
               && tmp != prev)
         {
            if (chunk_is_token(tmp, CT_COMMA) && tmp->level == prev->level + 1)
            {
               LOG_FMT(LFCN, "%s(%d): -- overriding call due to tuple return type -- prev is '%s', type is %s\n",
                       __func__, __LINE__, prev->text(), get_token_name(prev->type));
               isa_def = true;
               break;
            }
            tmp = chunk_get_next_ncnl(tmp);
         }
      }

      if (isa_def)
      {
         LOG_FMT(LFCN, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
         set_chunk_type(pc, CT_FUNC_DEF);
         LOG_FMT(LFCN, "%s(%d): type is set to FCN_DEF:\n",
                 __func__, __LINE__);

         if (prev == nullptr)
         {
            prev = chunk_get_head();
         }

         for (  tmp = prev; (tmp != nullptr)
             && tmp != pc; tmp = chunk_get_next_ncnl(tmp))
         {
            LOG_FMT(LFCN, "%s(%d): text() is '%s', type is %s\n",
                    __func__, __LINE__, tmp->text(), get_token_name(tmp->type));
            make_type(tmp);
         }
      }
   }

   if (pc->type != CT_FUNC_DEF)
   {
      LOG_FMT(LFCN, "%s(%d):  Detected type %s, text() is '%s', on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, get_token_name(pc->type),
              pc->text(), pc->orig_line, pc->orig_col);

      tmp = flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);

      if (  chunk_is_token(tmp, CT_BRACE_OPEN)
         && get_chunk_parent_type(tmp) != CT_DOUBLE_BRACE)
      {
         set_paren_parent(tmp, pc->type);
      }
      return;
   }
   /*
    * We have a function definition or prototype
    * Look for a semicolon or a brace open after the close parenthesis to figure
    * out whether this is a prototype or definition
    */

   // See if this is a prototype or implementation

   // FIXME: this doesn't take the old K&R parameter definitions into account

   // Scan tokens until we hit a brace open (def) or semicolon (proto)
   tmp = paren_close;

   while ((tmp = chunk_get_next_ncnl(tmp)) != nullptr)
   {
      // Only care about brace or semicolon on the same level
      if (tmp->level < pc->level)
      {
         // No semicolon - guess that it is a prototype
         chunk_flags_clr(pc, PCF_VAR_1ST_DEF);
         set_chunk_type(pc, CT_FUNC_PROTO);
         break;
      }
      else if (tmp->level == pc->level)
      {
         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            // its a function def for sure
            break;
         }
         else if (chunk_is_semicolon(tmp))
         {
            // Set the parent for the semicolon for later
            semi = tmp;
            chunk_flags_clr(pc, PCF_VAR_1ST_DEF);
            set_chunk_type(pc, CT_FUNC_PROTO);
            LOG_FMT(LFCN, "%s(%d):   2) Marked text() is '%s', as FUNC_PROTO on orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            break;
         }
         else if (chunk_is_token(pc, CT_COMMA))
         {
            set_chunk_type(pc, CT_FUNC_CTOR_VAR);
            LOG_FMT(LFCN, "%s(%d):   2) Marked text() is '%s', as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            break;
         }
      }
   }

   /*
    * C++ syntax is wacky. We need to check to see if a prototype is really a
    * variable definition with parameters passed into the constructor.
    * Unfortunately, without being able to accurately determine if an
    * identifier is a type (which would require us to more or less be a full
    * compiler), the only mostly reliable way to do so is to guess that it is
    * a constructor variable if inside a function body and scan the 'parameter
    * list' for items that are not allowed in a prototype. We search backwards
    * and checking the parent of the containing open braces. If the parent is a
    * class or namespace, then it probably is a prototype.
    */
   if (  language_is_set(LANG_CPP)
      && chunk_is_token(pc, CT_FUNC_PROTO)
      && get_chunk_parent_type(pc) != CT_OPERATOR)
   {
      LOG_FMT(LFPARAM, "%s(%d):", __func__, __LINE__);
      LOG_FMT(LFPARAM, "  checking '%s' for constructor variable %s %s\n",
              pc->text(),
              get_token_name(paren_open->type),
              get_token_name(paren_close->type));

      /*
       * Check the token at the start of the statement. If it's 'extern', we
       * definitely have a function prototype.
       */
      tmp = pc;

      while (tmp != nullptr && !tmp->flags.test(PCF_STMT_START))
      {
         tmp = chunk_get_prev_ncnlni(tmp);   // Issue #2279
      }
      const bool is_extern = (tmp && tmp->str.equals("extern"));

      /*
       * Scan the parameters looking for:
       *  - constant strings
       *  - numbers
       *  - non-type fields
       *  - function calls
       */
      chunk_t *ref = chunk_get_next_ncnl(paren_open);
      chunk_t *tmp2;
      bool    is_param = true;
      tmp = ref;

      while (tmp != paren_close)
      {
         tmp2 = chunk_get_next_ncnl(tmp);

         if (chunk_is_token(tmp, CT_COMMA) && (tmp->level == (paren_open->level + 1)))
         {
            if (!can_be_full_param(ref, tmp))
            {
               is_param = false;
               break;
            }
            ref = tmp2;
         }
         tmp = tmp2;
      }

      if (!is_extern && is_param && ref != tmp)
      {
         if (!can_be_full_param(ref, tmp))
         {
            is_param = false;
         }
      }

      if (!is_extern && !is_param)
      {
         set_chunk_type(pc, CT_FUNC_CTOR_VAR);
         LOG_FMT(LFCN, "%s(%d):   3) Marked text() '%s' as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      }
      else if (pc->brace_level > 0)
      {
         chunk_t *br_open = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->brace_level - 1);

         if (  br_open != nullptr
            && get_chunk_parent_type(br_open) != CT_EXTERN
            && get_chunk_parent_type(br_open) != CT_NAMESPACE)
         {
            // Do a check to see if the level is right
            prev = chunk_get_prev_ncnlni(pc);   // Issue #2279

            if (!chunk_is_str(prev, "*", 1) && !chunk_is_str(prev, "&", 1))
            {
               chunk_t *p_op = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->brace_level - 1);

               if (  p_op != nullptr
                  && get_chunk_parent_type(p_op) != CT_CLASS
                  && get_chunk_parent_type(p_op) != CT_STRUCT
                  && get_chunk_parent_type(p_op) != CT_NAMESPACE)
               {
                  set_chunk_type(pc, CT_FUNC_CTOR_VAR);
                  LOG_FMT(LFCN, "%s(%d):   4) Marked text() is'%s', as FUNC_CTOR_VAR on orig_line %zu, orig_col %zu\n",
                          __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               }
            }
         }
      }
   }

   if (semi != nullptr)
   {
      set_chunk_parent(semi, pc->type);
   }

   // Issue # 1403, 2152
   if (chunk_is_token(paren_open->prev, CT_FUNC_CTOR_VAR))
   {
      flag_parens(paren_open, PCF_IN_FCN_CTOR, CT_FPAREN_OPEN, pc->type, false);
   }
   else
   {
      flag_parens(paren_open, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, pc->type, false);
   }
   //flag_parens(paren_open, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, pc->type, true);

   if (chunk_is_token(pc, CT_FUNC_CTOR_VAR))
   {
      chunk_flags_set(pc, PCF_VAR_1ST_DEF);
      return;
   }

   if (chunk_is_token(next, CT_TSQUARE))
   {
      next = chunk_get_next_ncnl(next);

      if (next == nullptr)
      {
         return;
      }
   }
   // Mark parameters and return type
   fix_fcn_def_params(next);
   mark_function_return_type(pc, chunk_get_prev_ncnlni(pc), pc->type);   // Issue #2279

   /* mark C# where chunk */
   if (  language_is_set(LANG_CS)
      && ((chunk_is_token(pc, CT_FUNC_DEF)) || (chunk_is_token(pc, CT_FUNC_PROTO))))
   {
      tmp = chunk_get_next_ncnl(paren_close);
      pcf_flags_t in_where_spec_flags = PCF_NONE;

      while (  (tmp != nullptr)
            && (tmp->type != CT_BRACE_OPEN) && (tmp->type != CT_SEMICOLON))
      {
         mark_where_chunk(tmp, pc->type, tmp->flags | in_where_spec_flags);
         in_where_spec_flags = tmp->flags & PCF_IN_WHERE_SPEC;

         tmp = chunk_get_next_ncnl(tmp);
      }
   }

   // Find the brace pair and set the parent
   if (chunk_is_token(pc, CT_FUNC_DEF))
   {
      tmp = chunk_get_next_ncnl(paren_close);

      while (tmp != nullptr && tmp->type != CT_BRACE_OPEN)
      {
         //LOG_FMT(LSYS, "%s: set parent to FUNC_DEF on line %d: [%s]\n", __func__, tmp->orig_line, tmp->text());
         set_chunk_parent(tmp, CT_FUNC_DEF);

         if (!chunk_is_semicolon(tmp))
         {
            chunk_flags_set(tmp, PCF_OLD_FCN_PARAMS);
         }
         tmp = chunk_get_next_ncnl(tmp);
      }

      if (chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         set_chunk_parent(tmp, CT_FUNC_DEF);
         tmp = chunk_skip_to_match(tmp);

         if (tmp != nullptr)
         {
            set_chunk_parent(tmp, CT_FUNC_DEF);
         }
      }
   }
} // mark_function


static void mark_cpp_constructor(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *paren_open;
   chunk_t *tmp;
   chunk_t *after;
   chunk_t *var;
   bool    is_destr = false;

   tmp = chunk_get_prev_ncnlni(pc);   // Issue #2279

   if (chunk_is_token(tmp, CT_INV) || chunk_is_token(tmp, CT_DESTRUCTOR))
   {
      set_chunk_type(tmp, CT_DESTRUCTOR);
      set_chunk_parent(pc, CT_DESTRUCTOR);
      is_destr = true;
   }
   LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, FOUND %sSTRUCTOR for '%s'[%s] prev '%s'[%s]\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col,
           is_destr ? "DE" : "CON",
           pc->text(), get_token_name(pc->type),
           tmp->text(), get_token_name(tmp->type));

   paren_open = skip_template_next(chunk_get_next_ncnl(pc));

   if (!chunk_is_str(paren_open, "(", 1))
   {
      LOG_FMT(LWARN, "%s:%zu Expected '(', got: [%s]\n",
              cpd.filename.c_str(), paren_open->orig_line,
              paren_open->text());
      return;
   }
   // Mark parameters
   fix_fcn_def_params(paren_open);
   after = flag_parens(paren_open, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CLASS_PROTO, false);

   LOG_FMT(LFTOR, "%s(%d): text() '%s'\n", __func__, __LINE__, after->text());

   // Scan until the brace open, mark everything
   tmp = paren_open;
   bool hit_colon = false;

   while (  tmp != nullptr
         && (tmp->type != CT_BRACE_OPEN || tmp->level != paren_open->level)
         && !chunk_is_semicolon(tmp))
   {
      LOG_FMT(LFTOR, "%s(%d): tmp is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col);
      chunk_flags_set(tmp, PCF_IN_CONST_ARGS);
      tmp = chunk_get_next_ncnl(tmp);

      if (chunk_is_str(tmp, ":", 1) && tmp->level == paren_open->level)
      {
         set_chunk_type(tmp, CT_CONSTR_COLON);
         hit_colon = true;
      }

      if (  hit_colon
         && (chunk_is_paren_open(tmp) || chunk_is_opening_brace(tmp))
         && tmp->level == paren_open->level)
      {
         var = skip_template_prev(chunk_get_prev_ncnlni(tmp));   // Issue #2279

         if (chunk_is_token(var, CT_TYPE) || chunk_is_token(var, CT_WORD))
         {
            set_chunk_type(var, CT_FUNC_CTOR_VAR);
            flag_parens(tmp, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CTOR_VAR, false);
         }
      }
   }

   if (chunk_is_token(tmp, CT_BRACE_OPEN))
   {
      set_paren_parent(paren_open, CT_FUNC_CLASS_DEF);
      set_paren_parent(tmp, CT_FUNC_CLASS_DEF);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_DEF on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   }
   else
   {
      set_chunk_parent(tmp, CT_FUNC_CLASS_PROTO);
      set_chunk_type(pc, CT_FUNC_CLASS_PROTO);
      LOG_FMT(LFCN, "%s(%d):  Marked '%s' as FUNC_CLASS_PROTO on orig_line %zu, orig_col %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
   }
} // mark_cpp_constructor


static pcf_flags_t mark_where_chunk(chunk_t *pc, c_token_t parent_type, pcf_flags_t flags)
{
   /* TODO: should have options to control spacing around the ':' as well as newline ability for the
    * constraint clauses (should it break up a 'where A : B where C : D' on the same line? wrap? etc.) */

   if (chunk_is_token(pc, CT_WHERE))
   {
      set_chunk_type(pc, CT_WHERE_SPEC);
      set_chunk_parent(pc, parent_type);
      flags |= PCF_IN_WHERE_SPEC;
      LOG_FMT(LFTOR, "%s: where-spec on line %zu\n",
              __func__, pc->orig_line);
   }
   else if (flags.test(PCF_IN_WHERE_SPEC))
   {
      if (chunk_is_str(pc, ":", 1))
      {
         set_chunk_type(pc, CT_WHERE_COLON);
         LOG_FMT(LFTOR, "%s: where-spec colon on line %zu\n",
                 __func__, pc->orig_line);
      }
      else if ((chunk_is_token(pc, CT_STRUCT)) || (chunk_is_token(pc, CT_CLASS)))
      {
         /* class/struct inside of a where-clause confuses parser for indentation; set it as a word so it looks like the rest */
         set_chunk_type(pc, CT_WORD);
      }
   }

   if (flags.test(PCF_IN_WHERE_SPEC))
   {
      chunk_flags_set(pc, PCF_IN_WHERE_SPEC);
   }
   return(flags);
}


static void mark_class_ctor(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, start is '%s', parent_type is %s\n",
           __func__, __LINE__, start->orig_line, start->orig_col, start->text(),
           get_token_name(get_chunk_parent_type(start)));
   log_pcf_flags(LFTOR, start->flags);

   chunk_t *pclass = chunk_get_next_ncnl(start, scope_e::PREPROC);
   LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
           __func__, __LINE__, pclass->text());
   log_pcf_flags(LFTOR, pclass->flags);

   if (get_chunk_parent_type(start) == CT_TEMPLATE)
   {
      // look after the class name
      chunk_t *openingTemplate = chunk_get_next_ncnl(pclass);
      LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, openingTemplate is '%s', type is %s\n",
              __func__, __LINE__, openingTemplate->orig_line, openingTemplate->orig_col,
              openingTemplate->text(), get_token_name(openingTemplate->type));

      if (chunk_is_token(openingTemplate, CT_ANGLE_OPEN))
      {
         chunk_t *closingTemplate = chunk_skip_to_match(openingTemplate);
         LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, closingTemplate is '%s', type is %s\n",
                 __func__, __LINE__, closingTemplate->orig_line, closingTemplate->orig_col,
                 closingTemplate->text(), get_token_name(closingTemplate->type));
         chunk_t *thirdToken = chunk_get_next_ncnl(closingTemplate);
         LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, thirdToken is '%s', type is %s\n",
                 __func__, __LINE__, thirdToken->orig_line, thirdToken->orig_col,
                 thirdToken->text(), get_token_name(thirdToken->type));

         if (chunk_is_token(thirdToken, CT_DC_MEMBER))
         {
            pclass = chunk_get_next_ncnl(thirdToken);
            LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, orig_col is %zu, pclass is '%s', type is %s\n",
                    __func__, __LINE__, pclass->orig_line, pclass->orig_col,
                    pclass->text(), get_token_name(pclass->type));
         }
      }
   }
   pclass = skip_attribute_next(pclass);
   LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
           __func__, __LINE__, pclass->text());

   if (chunk_is_token(pclass, CT_DECLSPEC))  // Issue 1289
   {
      pclass = chunk_get_next_ncnl(pclass);
      LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
              __func__, __LINE__, pclass->text());

      if (chunk_is_token(pclass, CT_PAREN_OPEN))
      {
         pclass = chunk_get_next_ncnl(chunk_skip_to_match(pclass));
         LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
                 __func__, __LINE__, pclass->text());
      }
   }

   if (  pclass == nullptr
      || (pclass->type != CT_TYPE && pclass->type != CT_WORD))
   {
      return;
   }
   chunk_t *next = chunk_get_next_ncnl(pclass, scope_e::PREPROC);

   while (  chunk_is_token(next, CT_TYPE)
         || chunk_is_token(next, CT_WORD)
         || chunk_is_token(next, CT_DC_MEMBER))
   {
      pclass = next;
      LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
              __func__, __LINE__, pclass->text());
      next = chunk_get_next_ncnl(next, scope_e::PREPROC);
   }
   chunk_t *pc   = chunk_get_next_ncnl(pclass, scope_e::PREPROC);
   size_t  level = pclass->brace_level + 1;

   if (pc == nullptr)
   {
      LOG_FMT(LFTOR, "%s(%d): Called on %s on orig_line %zu. Bailed on NULL\n",
              __func__, __LINE__, pclass->text(), pclass->orig_line);
      return;
   }
   // Add the class name
   ChunkStack cs;
   cs.Push_Back(pclass);

   LOG_FMT(LFTOR, "%s(%d): Called on %s on orig_line %zu (next is '%s')\n",
           __func__, __LINE__, pclass->text(), pclass->orig_line, pc->text());

   // detect D template class: "class foo(x) { ... }"
   if (language_is_set(LANG_D) && chunk_is_token(next, CT_PAREN_OPEN))              // Coverity CID 76004
   {
      set_chunk_parent(next, CT_TEMPLATE);

      next = get_d_template_types(cs, next);

      if (chunk_is_token(next, CT_PAREN_CLOSE))
      {
         set_chunk_parent(next, CT_TEMPLATE);
      }
   }
   // Find the open brace, abort on semicolon
   pcf_flags_t flags = PCF_NONE;

   while (pc != nullptr && pc->type != CT_BRACE_OPEN)
   {
      LOG_FMT(LFTOR, " [%s]", pc->text());

      flags = mark_where_chunk(pc, start->type, flags);

      if (!flags.test(PCF_IN_WHERE_SPEC) && chunk_is_str(pc, ":", 1))
      {
         set_chunk_type(pc, CT_CLASS_COLON);
         flags |= PCF_IN_CLASS_BASE;
         LOG_FMT(LFTOR, "%s(%d): class colon on line %zu\n",
                 __func__, __LINE__, pc->orig_line);
      }

      if (chunk_is_semicolon(pc))
      {
         LOG_FMT(LFTOR, "%s(%d): bailed on semicolon on line %zu\n",
                 __func__, __LINE__, pc->orig_line);
         return;
      }
      chunk_flags_set(pc, flags);
      pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
   }

   if (pc == nullptr)
   {
      LOG_FMT(LFTOR, "%s(%d): bailed on NULL\n", __func__, __LINE__);
      return;
   }
   set_paren_parent(pc, start->type);
   chunk_flags_set(pc, PCF_IN_CLASS);

   pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);
   LOG_FMT(LFTOR, "%s(%d): pclass is '%s'\n",
           __func__, __LINE__, pclass->text());

   while (pc != nullptr)
   {
      LOG_FMT(LFTOR, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
      chunk_flags_set(pc, PCF_IN_CLASS);

      if (  pc->brace_level > level
         || pc->level > pc->brace_level
         || pc->flags.test(PCF_IN_PREPROC))
      {
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if (chunk_is_token(pc, CT_BRACE_CLOSE) && pc->brace_level < level)
      {
         LOG_FMT(LFTOR, "%s(%d): orig_line is %zu, Hit brace close\n",
                 __func__, __LINE__, pc->orig_line);
         pc = chunk_get_next_ncnl(pc, scope_e::PREPROC);

         if (chunk_is_token(pc, CT_SEMICOLON))
         {
            set_chunk_parent(pc, start->type);
         }
         return;
      }
      next = chunk_get_next_ncnl(pc, scope_e::PREPROC);

      if (chunkstack_match(cs, pc))
      {
         LOG_FMT(LFTOR, "%s(%d): pc is '%s', orig_line is %zu, orig_col is %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
         // Issue #1333 Formatter removes semicolon after variable initializer at class level(C#)
         // if previous chunk is 'new' operator it is variable initializer not a CLASS_FUNC_DEF.
         chunk_t *prev = chunk_get_prev_ncnlni(pc, scope_e::PREPROC);   // Issue #2279
         LOG_FMT(LFTOR, "%s(%d): prev is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                 __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col, get_token_name(prev->type));

         // Issue #1003, next->type should not be CT_FPAREN_OPEN
         if (  prev != nullptr
            && (prev->type != CT_NEW))
         {
            bool is_func_class_def = false;

            if (chunk_is_token(next, CT_PAREN_OPEN))
            {
               is_func_class_def = true;
            }
            else if (chunk_is_token(next, CT_ANGLE_OPEN))          // Issue # 1737
            {
               chunk_t *closeAngle    = chunk_skip_to_match(next);
               chunk_t *afterTemplate = chunk_get_next(closeAngle);

               if (chunk_is_token(afterTemplate, CT_PAREN_OPEN))
               {
                  is_func_class_def = true;
               }
            }
            else
            {
               LOG_FMT(LFTOR, "%s(%d): text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
               make_type(pc);
            }

            if (is_func_class_def)
            {
               set_chunk_type(pc, CT_FUNC_CLASS_DEF);
               LOG_FMT(LFTOR, "%s(%d): text() is '%s', orig_line is %zu, orig_col is %zu, type is %s, Marked CTor/DTor\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col, get_token_name(pc->type));
               mark_cpp_constructor(pc);
            }
         }
      }
      pc = next;
   }
} // mark_class_ctor


static chunk_t *skip_align(chunk_t *start)
{
   chunk_t *pc = start;

   if (chunk_is_token(pc, CT_ALIGN))
   {
      pc = chunk_get_next_ncnl(pc);

      if (chunk_is_token(pc, CT_PAREN_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
         pc = chunk_get_next_ncnl(pc);

         if (chunk_is_token(pc, CT_COLON))
         {
            pc = chunk_get_next_ncnl(pc);
         }
      }
   }
   return(pc);
}


chunk_t *skip_parent_types(chunk_t *colon)
{
   auto pc = chunk_get_next_ncnlnp(colon);

   while (pc)
   {
      // Skip access specifier
      if (chunk_is_token(pc, CT_ACCESS))
      {
         pc = chunk_get_next_ncnlnp(pc);
         continue;
      }

      // Check for a type name
      if (!(chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, CT_TYPE)))
      {
         LOG_FMT(LPCU,
                 "%s is confused; expected a word at %zu:%zu "
                 "following type list at %zu:%zu\n", __func__,
                 colon->orig_line, colon->orig_col,
                 pc->orig_line, pc->orig_col);
         return(colon);
      }
      // Get next token
      auto next = skip_template_next(chunk_get_next_ncnlnp(pc));

      if (chunk_is_token(next, CT_DC_MEMBER) || chunk_is_token(next, CT_COMMA))
      {
         pc = chunk_get_next_ncnlnp(next);
      }
      else if (next)
      {
         LOG_FMT(LPCU, "%s -> %zu:%zu ('%s')\n", __func__,
                 next->orig_line, next->orig_col, next->text());
         return(next);
      }
      else
      {
         break;
      }
   }
   LOG_FMT(LPCU, "%s: did not find end of type list (start was %zu:%zu)\n",
           __func__, colon->orig_line, colon->orig_col);
   return(colon);
} // skip_parent_types


static void mark_struct_union_body(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   while (  pc != nullptr
         && pc->level >= start->level
         && !(pc->level == start->level && chunk_is_token(pc, CT_BRACE_CLOSE)))
   {
      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_SEMICOLON))
      {
         pc = chunk_get_next_ncnl(pc);

         if (pc == nullptr)
         {
            break;
         }
      }

      if (chunk_is_token(pc, CT_ALIGN))
      {
         pc = skip_align(pc); // "align(x)" or "align(x):"

         if (pc == nullptr)
         {
            break;
         }
      }
      else
      {
         pc = fix_var_def(pc);

         if (pc == nullptr)
         {
            break;
         }
      }
   }
} // mark_struct_union_body


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


static void mark_define_expressions(void)
{
   LOG_FUNC_ENTRY();

   bool    in_define = false;
   bool    first     = true;
   chunk_t *pc       = chunk_get_head();
   chunk_t *prev     = pc;

   while (pc != nullptr)
   {
      if (!in_define)
      {
         if (  chunk_is_token(pc, CT_PP_DEFINE)
            || chunk_is_token(pc, CT_PP_IF)
            || chunk_is_token(pc, CT_PP_ELSE))
         {
            in_define = true;
            first     = true;
         }
      }
      else
      {
         if (!pc->flags.test(PCF_IN_PREPROC) || chunk_is_token(pc, CT_PREPROC))
         {
            in_define = false;
         }
         else
         {
            if (  pc->type != CT_MACRO
               && (  first
                  || chunk_is_token(prev, CT_PAREN_OPEN)
                  || chunk_is_token(prev, CT_ARITH)
                  || chunk_is_token(prev, CT_CARET)
                  || chunk_is_token(prev, CT_ASSIGN)
                  || chunk_is_token(prev, CT_COMPARE)
                  || chunk_is_token(prev, CT_RETURN)
                  || chunk_is_token(prev, CT_GOTO)
                  || chunk_is_token(prev, CT_CONTINUE)
                  || chunk_is_token(prev, CT_FPAREN_OPEN)
                  || chunk_is_token(prev, CT_SPAREN_OPEN)
                  || chunk_is_token(prev, CT_BRACE_OPEN)
                  || chunk_is_semicolon(prev)
                  || chunk_is_token(prev, CT_COMMA)
                  || chunk_is_token(prev, CT_COLON)
                  || chunk_is_token(prev, CT_QUESTION)))
            {
               chunk_flags_set(pc, PCF_EXPR_START);
               first = false;
            }
         }
      }
      prev = pc;
      pc   = chunk_get_next(pc);
   }
} // mark_define_expressions


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
   // lambda-declarator '( params )' is optional
   chunk_t *pa_o = chunk_get_next_ncnl(sq_c);

   if (pa_o == nullptr)
   {
      return;
   }
   chunk_t *pa_c = nullptr;

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

   if (pa_c)
   {
      set_chunk_type(pa_o, CT_FPAREN_OPEN);
      set_chunk_parent(pa_o, CT_CPP_LAMBDA);
      set_chunk_type(pa_c, CT_FPAREN_CLOSE);
      set_chunk_parent(pa_c, CT_CPP_LAMBDA);
   }
   set_chunk_parent(br_o, CT_CPP_LAMBDA);
   set_chunk_parent(br_c, CT_CPP_LAMBDA);

   if (ret)
   {
      set_chunk_type(ret, CT_CPP_LAMBDA_RET);
      ret = chunk_get_next_ncnl(ret);

      while (ret != br_o)
      {
         make_type(ret);
         ret = chunk_get_next_ncnl(ret);
      }
   }

   if (pa_c)
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


static chunk_t *get_d_template_types(ChunkStack &cs, chunk_t *open_paren)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp       = open_paren;
   bool    maybe_type = true;

   while (  ((tmp = chunk_get_next_ncnl(tmp)) != nullptr)
         && tmp->level > open_paren->level)
   {
      if (chunk_is_token(tmp, CT_TYPE) || chunk_is_token(tmp, CT_WORD))
      {
         if (maybe_type)
         {
            make_type(tmp);
            cs.Push_Back(tmp);
         }
         maybe_type = false;
      }
      else if (chunk_is_token(tmp, CT_COMMA))
      {
         maybe_type = true;
      }
   }
   return(tmp);
}


static bool chunkstack_match(ChunkStack &cs, chunk_t *pc)
{
   for (size_t idx = 0; idx < cs.Len(); idx++)
   {
      chunk_t *tmp = cs.GetChunk(idx);

      if (pc->str.equals(tmp->str))
      {
         return(true);
      }
   }

   return(false);
}


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


static void mark_template_func(chunk_t *pc, chunk_t *pc_next)
{
   LOG_FUNC_ENTRY();

   // We know angle_close must be there...
   chunk_t *angle_close = chunk_get_next_type(pc_next, CT_ANGLE_CLOSE, pc->level);
   chunk_t *after       = chunk_get_next_ncnl(angle_close);

   if (after != nullptr)
   {
      if (chunk_is_str(after, "(", 1))
      {
         if (angle_close->flags.test(PCF_IN_FCN_CALL))
         {
            LOG_FMT(LTEMPFUNC, "%s(%d): marking '%s' in line %zu as a FUNC_CALL\n",
                    __func__, __LINE__, pc->text(), pc->orig_line);
            set_chunk_type(pc, CT_FUNC_CALL);
            flag_parens(after, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
         }
         else
         {
            /*
             * Might be a function def. Must check what is before the template:
             * Func call:
             *   BTree.Insert(std::pair<int, double>(*it, double(*it) + 1.0));
             *   a = Test<int>(j);
             *   std::pair<int, double>(*it, double(*it) + 1.0));
             */

            LOG_FMT(LTEMPFUNC, "%s(%d): marking '%s' in line %zu as a FUNC_CALL 2\n",
                    __func__, __LINE__, pc->text(), pc->orig_line);
            // its a function!!!
            set_chunk_type(pc, CT_FUNC_CALL);
            mark_function(pc);
         }
      }
      else if (chunk_is_token(after, CT_WORD))
      {
         // its a type!
         set_chunk_type(pc, CT_TYPE);
         chunk_flags_set(pc, PCF_VAR_TYPE);
         chunk_flags_set(after, PCF_VAR_DEF);
      }
   }
}


static void mark_exec_sql(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   // Change CT_WORD to CT_SQL_WORD
   for (tmp = chunk_get_next(pc); tmp != nullptr; tmp = chunk_get_next(tmp))
   {
      set_chunk_parent(tmp, pc->type);

      if (chunk_is_token(tmp, CT_WORD))
      {
         set_chunk_type(tmp, CT_SQL_WORD);
      }

      if (chunk_is_token(tmp, CT_SEMICOLON))
      {
         break;
      }
   }

   if (  pc->type != CT_SQL_BEGIN
      || tmp == nullptr
      || tmp->type != CT_SEMICOLON)
   {
      return;
   }

   for (tmp = chunk_get_next(tmp);
        tmp != nullptr && tmp->type != CT_SQL_END;
        tmp = chunk_get_next(tmp))
   {
      tmp->level++;
   }
}


chunk_t *skip_template_next(chunk_t *ang_open)
{
   if (chunk_is_token(ang_open, CT_ANGLE_OPEN))
   {
      chunk_t *pc = chunk_get_next_type(ang_open, CT_ANGLE_CLOSE, ang_open->level);
      return(chunk_get_next_ncnl(pc));
   }
   return(ang_open);
}


chunk_t *skip_template_prev(chunk_t *ang_close)
{
   if (chunk_is_token(ang_close, CT_ANGLE_CLOSE))
   {
      chunk_t *pc = chunk_get_prev_type(ang_close, CT_ANGLE_OPEN, ang_close->level);
      return(chunk_get_prev_ncnlni(pc));   // Issue #2279
   }
   return(ang_close);
}


chunk_t *skip_tsquare_next(chunk_t *ary_def)
{
   if (chunk_is_token(ary_def, CT_SQUARE_OPEN) || chunk_is_token(ary_def, CT_TSQUARE))
   {
      return(chunk_get_next_nisq(ary_def));
   }
   return(ary_def);
}


chunk_t *skip_attribute_next(chunk_t *attr)
{
   if (chunk_is_token(attr, CT_ATTRIBUTE))
   {
      chunk_t *pc = chunk_get_next(attr);

      if (chunk_is_token(pc, CT_FPAREN_OPEN))
      {
         pc = chunk_get_next_type(attr, CT_FPAREN_CLOSE, attr->level);
         return(chunk_get_next_ncnl(pc));
      }
      return(pc);
   }
   return(attr);
}


chunk_t *skip_attribute_prev(chunk_t *fp_close)
{
   if (  chunk_is_token(fp_close, CT_FPAREN_CLOSE)
      && get_chunk_parent_type(fp_close) == CT_ATTRIBUTE)
   {
      chunk_t *pc = chunk_get_prev_type(fp_close, CT_ATTRIBUTE, fp_close->level);
      return(chunk_get_prev_ncnlni(pc));   // Issue #2279
   }
   return(fp_close);
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
           && tmp->type != CT_STRING)
   {
      LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu, expected identifier, not '%s' [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col,
              tmp->text(), get_token_name(tmp->type));
      return;
   }
   else
   {
      chunk_t *tt = chunk_get_next_ncnl(tmp);

      if (chunk_is_paren_open(tt))
      {
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
   fix_var_def(tmp);
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


void remove_extra_returns(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc = chunk_get_head();

   while (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_RETURN) && !pc->flags.test(PCF_IN_PREPROC))
      {
         chunk_t *semi  = chunk_get_next_ncnl(pc);
         chunk_t *cl_br = chunk_get_next_ncnl(semi);

         if (  chunk_is_token(semi, CT_SEMICOLON)
            && chunk_is_token(cl_br, CT_BRACE_CLOSE)
            && (  get_chunk_parent_type(cl_br) == CT_FUNC_DEF
               || get_chunk_parent_type(cl_br) == CT_FUNC_CLASS_DEF))
         {
            LOG_FMT(LRMRETURN, "%s(%d): Removed 'return;' on orig_line %zu\n",
                    __func__, __LINE__, pc->orig_line);
            chunk_del(pc);
            chunk_del(semi);
            pc = cl_br;
         }
      }
      pc = chunk_get_next(pc);
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
