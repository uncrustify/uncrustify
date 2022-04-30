/**
 * @file combine.cpp
 * Labels the chunks as needed.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "combine.h"

#include "combine_fix_mark.h"
#include "combine_skip.h"
#include "combine_tools.h"
#include "EnumStructUnionParser.h"
#include "flag_braced_init_list.h"
#include "flag_parens.h"
#include "lang_pawn.h"
#include "newlines.h"
#include "prototypes.h"
#include "tokenize_cleanup.h"

#include <limits>

constexpr static auto LCURRENT = LCOMBINE;

using namespace std;
using namespace uncrustify;


/**
 * Mark the parens and colons in:
 *   asm volatile ( "xx" : "xx" (l), "yy"(h) : ...  );
 *
 * @param pc  the CT_ASM item
 */
static void flag_asm(Chunk *pc);


/**
 * Skips the list of class/struct parent types.
 */
Chunk *skip_parent_types(Chunk *colon);


/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
static void check_double_brace_init(Chunk *bo1);


static void process_returns(void);


/**
 * Processes a return statement, labeling the parens and marking the parent.
 * May remove or add parens around the return statement
 *
 * @param pc  Pointer to the return chunk
 */
static Chunk *process_return(Chunk *pc);


/**
 * Process an ObjC 'class'
 * pc is the chunk after '@implementation' or '@interface' or '@protocol'.
 * Change colons, etc. Processes stuff until '@end'.
 * Skips anything in braces.
 */
static void handle_oc_class(Chunk *pc);


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
static void handle_oc_block_literal(Chunk *pc);


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
static void handle_oc_block_type(Chunk *pc);


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
static void handle_oc_message_decl(Chunk *pc);


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
static void handle_oc_message_send(Chunk *pc);


//! Process @Property values and re-arrange them if necessary
static void handle_oc_property_decl(Chunk *pc);

//! Process @available annotation
static void handle_oc_available(Chunk *pc);

/**
 * Process a type that is enclosed in parens in message declarations.
 * TODO: handle block types, which get special formatting
 *
 * @param pc  points to the open paren
 *
 * @return the chunk after the type
 */
static Chunk *handle_oc_md_type(Chunk *paren_open, E_Token ptype, pcf_flags_t flags, bool &did_it);

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
static void handle_cs_square_stmt(Chunk *pc);


/**
 * We are on a brace open that is preceded by a word or square close.
 * Set the brace parent to CT_CS_PROPERTY and find the first item in the
 * property and set its parent, too.
 */
static void handle_cs_property(Chunk *pc);


/**
 * We hit a ']' followed by a WORD. This may be a multidimensional array type.
 * Example: int[,,] x;
 * If there is nothing but commas between the open and close, then mark it.
 */
static void handle_cs_array_type(Chunk *pc);


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
static void handle_cpp_template(Chunk *pc);


/**
 * Verify and then mark C++ lambda expressions.
 * The expected format is '[...](...){...}' or '[...](...) -> type {...}'
 * sq_o is '[' CT_SQUARE_OPEN or '[]' CT_TSQUARE
 * Split the '[]' so we can control the space
 */
static void handle_cpp_lambda(Chunk *pc);


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
static void handle_d_template(Chunk *pc);


/**
 * A func wrap chunk and what follows should be treated as a function name.
 * Create new text for the chunk and call it a CT_FUNCTION.
 *
 * A type wrap chunk and what follows should be treated as a simple type.
 * Create new text for the chunk and call it a CT_TYPE.
 */
static void handle_wrap(Chunk *pc);


/**
 * A proto wrap chunk and what follows should be treated as a function proto.
 *
 * RETTYPE PROTO_WRAP( NAME, PARAMS ); or RETTYPE PROTO_WRAP( NAME, (PARAMS) );
 * RETTYPE gets changed with make_type().
 * PROTO_WRAP is marked as CT_FUNC_PROTO or CT_FUNC_DEF.
 * NAME is marked as CT_WORD.
 * PARAMS is all marked as prototype parameters.
 */
static void handle_proto_wrap(Chunk *pc);


static bool is_oc_block(Chunk *pc);


/**
 * Java assert statements are: "assert EXP1 [: EXP2] ;"
 * Mark the parent of the colon and semicolon
 */
static void handle_java_assert(Chunk *pc);


static void flag_asm(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *tmp = pc->GetNextNcNnl(E_Scope::PREPROC);

   if (chunk_is_not_token(tmp, CT_QUALIFIER))
   {
      return;
   }
   Chunk *po = tmp->GetNextNcNnl(E_Scope::PREPROC);

   if (!chunk_is_paren_open(po))
   {
      return;
   }
   Chunk *end = chunk_skip_to_match(po, E_Scope::PREPROC);

   if (end == nullptr)
   {
      return;
   }
   set_chunk_parent(po, CT_ASM);
   set_chunk_parent(end, CT_ASM);

   for (  tmp = po->GetNextNcNnl(E_Scope::PREPROC);
          tmp->IsNotNullChunk()
       && tmp != end;
          tmp = tmp->GetNextNcNnl(E_Scope::PREPROC))
   {
      if (chunk_is_token(tmp, CT_COLON))
      {
         set_chunk_type(tmp, CT_ASM_COLON);
      }
      else if (chunk_is_token(tmp, CT_DC_MEMBER))
      {
         // if there is a string on both sides, then this is two ASM_COLONs
         if (  chunk_is_token(tmp->GetNextNcNnl(E_Scope::PREPROC), CT_STRING)
            && chunk_is_token(tmp->GetPrevNcNnlNi(E_Scope::PREPROC), CT_STRING)) // Issue #2279
         {
            Chunk nc;

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

   tmp = end->GetNextNcNnl(E_Scope::PREPROC);

   if (tmp->IsNullChunk())
   {
      return;
   }

   if (chunk_is_token(tmp, CT_SEMICOLON))
   {
      set_chunk_parent(tmp, CT_ASM);
   }
} // flag_asm


void do_symbol_check(Chunk *prev, Chunk *pc, Chunk *next)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFCNR, "%s(%d): prev is '%s' %s\n",
           __func__, __LINE__,
           prev->Text(), get_token_name(prev->type));
   log_pcf_flags(LFCNR, prev->flags);
   LOG_FMT(LFCNR, "%s(%d): pc is '%s' %s\n",
           __func__, __LINE__,
           pc->Text(), get_token_name(pc->type));
   log_pcf_flags(LFCNR, pc->flags);
   LOG_FMT(LFCNR, "%s(%d): next is '%s' %s\n",
           __func__, __LINE__,
           next->Text(), get_token_name(next->type));
   log_pcf_flags(LFCNR, next->flags);

   if (  chunk_is_token(pc, CT_NOEXCEPT)                 // Issue #3284
      && chunk_is_token(next, CT_ASSIGN))                // skip over noexcept
   {
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      pc   = next;
      next = pc->GetNext();
   }

   // separate the uses of CT_ASSIGN sign '='
   // into CT_ASSIGN_DEFAULT_ARG, CT_ASSIGN_FUNC_PROTO
   if (  chunk_is_token(pc, CT_ASSIGN)
      && get_chunk_parent_type(pc) == CT_FUNC_PROTO
      && (  pc->flags.test(PCF_IN_FCN_DEF)                            // Issue #2236
         || pc->flags.test(PCF_IN_CONST_ARGS)))
   {
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      log_pcf_flags(LFCNR, pc->flags);
      set_chunk_type(pc, CT_ASSIGN_DEFAULT_ARG);
      return;
   }

   if (  (  chunk_is_token(prev, CT_FPAREN_CLOSE)
         || (  (  chunk_is_str(prev, "const")
               || chunk_is_str(prev, "override"))
            && chunk_is_token(prev->prev, CT_FPAREN_CLOSE)))
      && chunk_is_token(pc, CT_ASSIGN)
      && (  chunk_is_token(next, CT_DEFAULT)
         || chunk_is_token(next, CT_DELETE)
         || chunk_is_str(next, "0")))
   {
      set_chunk_type(pc, CT_ASSIGN_FUNC_PROTO);
      return;                  // cpp 30031
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
         return;                  // objective-c_50095
      }
   }

   // D stuff
   if (  language_is_set(LANG_D)
      && chunk_is_token(pc, CT_QUALIFIER)
      && chunk_is_str(pc, "const")
      && chunk_is_token(next, CT_PAREN_OPEN))
   {
      set_chunk_type(pc, CT_D_CAST);
      set_paren_parent(next, pc->type);
      return;                  // d_40061
   }

   if (  chunk_is_token(next, CT_PAREN_OPEN)
      && (  chunk_is_token(pc, CT_D_CAST)
         || chunk_is_token(pc, CT_DELEGATE)
         || chunk_is_token(pc, CT_ALIGN)))
   {
      // mark the parenthesis parent
      Chunk *tmp = set_paren_parent(next, pc->type);

      // For a D cast - convert the next item
      if (  chunk_is_token(pc, CT_D_CAST)
         && tmp != nullptr)
      {
         if (chunk_is_token(tmp, CT_STAR))
         {
            set_chunk_type(tmp, CT_DEREF);
            return;                  // d_40006
         }
         else if (chunk_is_token(tmp, CT_AMP))
         {
            set_chunk_type(tmp, CT_ADDR);
            return;                  // d_40060
         }
         else if (chunk_is_token(tmp, CT_MINUS))
         {
            set_chunk_type(tmp, CT_NEG);
            return;                  // d_40060
         }
         else if (chunk_is_token(tmp, CT_PLUS))
         {
            set_chunk_type(tmp, CT_POS);
            return;                  // d_40060
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

         for (tmp = pc->GetPrevNcNnlNi(); tmp->IsNotNullChunk(); tmp = tmp->GetPrevNcNnlNi()) // Issue #2279
         {
            if (  chunk_is_semicolon(tmp)
               || chunk_is_token(tmp, CT_BRACE_OPEN)
               || chunk_is_token(tmp, CT_VBRACE_OPEN))
            {
               break;
            }
            make_type(tmp);
         }

         return;                  // c-sharp_10160
      }

      if (  chunk_is_token(pc, CT_ALIGN)
         && tmp != nullptr)
      {
         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, pc->type);
            return;                  // d_40024
         }
         else if (chunk_is_token(tmp, CT_COLON))
         {
            set_chunk_parent(tmp, pc->type);
            return;                  // d_40024
         }
      }
   } // paren open + cast/align/delegate

   if (chunk_is_token(pc, CT_INVARIANT))
   {
      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_chunk_parent(next, pc->type);
         Chunk *tmp = next->GetNext();

         if (tmp == nullptr)
         {
            tmp = Chunk::NullChunkPtr;
         }

         while (tmp->IsNotNullChunk())
         {
            if (chunk_is_token(tmp, CT_PAREN_CLOSE))
            {
               set_chunk_parent(tmp, pc->type);
               break;
            }
            make_type(tmp);
            tmp = tmp->GetNext();
         }
         return;                  // d_40100
      }
      else
      {
         set_chunk_type(pc, CT_QUALIFIER);
         return;
      }
   }

   if (  chunk_is_token(prev, CT_BRACE_OPEN)
      && get_chunk_parent_type(prev) != CT_CS_PROPERTY
      && (  chunk_is_token(pc, CT_GETSET)
         || chunk_is_token(pc, CT_GETSET_EMPTY)))
   {
      flag_parens(prev, PCF_NONE, CT_NONE, CT_GETSET, false);
      return;
   }

   if (chunk_is_token(pc, CT_ASM))
   {
      flag_asm(pc);
      return;
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
            return;
         }
      }
   }

   // Objective C stuff
   if (language_is_set(LANG_OC))
   {
      // Check for message declarations
      if (pc->flags.test(PCF_STMT_START))
      {
         if (  (  chunk_is_str(pc, "-")
               || chunk_is_str(pc, "+"))
            && chunk_is_str(next, "("))
         {
            handle_oc_message_decl(pc);
            return;
         }
      }

      if (  pc->flags.test(PCF_EXPR_START)
         || pc->flags.test(PCF_IN_PREPROC))
      {
         if (chunk_is_token(pc, CT_SQUARE_OPEN))
         {
            handle_oc_message_send(pc);
            return;                  // objective-c_50003
         }
      }

      if (chunk_is_token(pc, CT_OC_PROPERTY))
      {
         handle_oc_property_decl(pc);
         return;
      }

      if (chunk_is_token(pc, CT_OC_AVAILABLE))
      {
         handle_oc_available(pc);
         return;
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
         return;
      }

      if (  chunk_is_token(next, CT_BRACE_OPEN)
         && get_chunk_parent_type(next) == CT_NONE
         && (  chunk_is_token(pc, CT_SQUARE_CLOSE)
            || chunk_is_token(pc, CT_ANGLE_CLOSE)
            || chunk_is_token(pc, CT_WORD)))
      {
         handle_cs_property(next);
         return;
      }

      if (  chunk_is_token(pc, CT_SQUARE_CLOSE)
         && chunk_is_token(next, CT_WORD))
      {
         handle_cs_array_type(pc);
         return;
      }

      if (  (  chunk_is_token(pc, CT_LAMBDA)
            || chunk_is_token(pc, CT_DELEGATE))
         && chunk_is_token(next, CT_BRACE_OPEN))
      {
         set_paren_parent(next, pc->type);
         return;
      }

      if (  chunk_is_token(pc, CT_WHEN)
         && pc->GetNext()->IsNotNullChunk()
         && pc->GetNext()->type != CT_SPAREN_OPEN)
      {
         set_chunk_type(pc, CT_WORD);
         return;
      }
   }

   if (  language_is_set(LANG_JAVA)
      && chunk_is_token(pc, CT_LAMBDA)
      && chunk_is_token(next, CT_BRACE_OPEN))
   {
      set_paren_parent(next, pc->type);
      return;
   }

   if (chunk_is_token(pc, CT_NEW))
   {
      Chunk *ts  = nullptr;
      Chunk *tmp = next;

      if (chunk_is_token(tmp, CT_TSQUARE))
      {
         ts  = tmp;
         tmp = tmp->GetNextNcNnl();
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
      return;
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
         Chunk *tmp = next;

         while ((tmp = tmp->GetNextNc())->IsNotNullChunk())
         {
            if (chunk_is_newline(tmp))
            {
               break;
            }

            if (  chunk_is_token(tmp, CT_SQUARE_CLOSE)
               && next->level == tmp->level)
            {
               chunk_flags_set(tmp, PCF_ONE_LINER);
               chunk_flags_set(next, PCF_ONE_LINER);
               break;
            }
         }
         return;
      }
   }

   if (chunk_is_token(pc, CT_ASSERT))
   {
      handle_java_assert(pc);
      return;
   }

   if (chunk_is_token(pc, CT_ANNOTATION))
   {
      Chunk *tmp = pc->GetNextNcNnl();

      if (chunk_is_paren_open(tmp))
      {
         set_paren_parent(tmp, CT_ANNOTATION);
      }
      return;
   }

   if (  chunk_is_token(pc, CT_SIZEOF)
      && language_is_set(LANG_ALLC))
   {
      Chunk *tmp = pc->GetNextNcNnl();

      if (chunk_is_token(tmp, CT_ELLIPSIS))
      {
         set_chunk_parent(tmp, CT_SIZEOF);
      }
      return;
   }

   if (  chunk_is_token(pc, CT_DECLTYPE)
      && pc->parent_type != CT_FUNC_DEF)
   {
      Chunk *tmp = pc->GetNextNcNnl();

      if (chunk_is_paren_open(tmp))
      {
         // decltype may be followed by a braced-init-list
         tmp = set_paren_parent(tmp, CT_DECLTYPE);

         if (chunk_is_opening_brace(tmp) && !pc->flags.test(PCF_IN_LAMBDA))
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
      return;
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
      return;
   }

   if (  chunk_is_token(pc, CT_SQL_EXEC)
      || chunk_is_token(pc, CT_SQL_BEGIN)
      || chunk_is_token(pc, CT_SQL_END))
   {
      mark_exec_sql(pc);
      return;
   }

   if (chunk_is_token(pc, CT_PROTO_WRAP))
   {
      handle_proto_wrap(pc);
      return;
   }

   // Handle the typedef
   if (chunk_is_token(pc, CT_TYPEDEF))
   {
      fix_typedef(pc);
      return;
   }

   if (  chunk_is_class_enum_struct_union(pc)
      && chunk_is_not_token(prev, CT_TYPEDEF))
   {
      EnumStructUnionParser parser;
      parser.parse(pc);
      return;
   }

   if (chunk_is_token(pc, CT_EXTERN))
   {
      if (chunk_is_paren_open(next))
      {
         Chunk *tmp = flag_parens(next, PCF_NONE, CT_NONE, CT_EXTERN, true);

         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, CT_EXTERN);
         }
      }
      else
      {
         // next likely is a string (see tokenize_cleanup.cpp)
         set_chunk_parent(next, CT_EXTERN);
         Chunk *tmp = next->GetNextNcNnl();

         if (chunk_is_token(tmp, CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, CT_EXTERN);
         }
      }
      return;
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
      return;
   }

   if (  chunk_is_token(pc, CT_WORD)
      && chunk_is_token(next, CT_ANGLE_OPEN)
      && get_chunk_parent_type(next) == CT_TEMPLATE)
   {
      mark_template_func(pc, next);
      return;
   }

   if (  chunk_is_token(pc, CT_SQUARE_CLOSE)
      && chunk_is_token(next, CT_PAREN_OPEN))
   {
      flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, CT_NONE, false);
      return;
   }

   if (chunk_is_token(pc, CT_TYPE_CAST))
   {
      fix_type_cast(pc);
      return;
   }

   if (  get_chunk_parent_type(pc) == CT_ASSIGN
      && (  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_SQUARE_OPEN)))
   {
      // Mark everything in here as in assign
      flag_parens(pc, PCF_IN_ARRAY_ASSIGN, pc->type, CT_NONE, false);
      return;
   }

   if (chunk_is_token(pc, CT_D_TEMPLATE))
   {
      set_paren_parent(next, pc->type);
      return;
   }

   /*
    * A word before an open paren is a function call or definition.
    * CT_WORD => CT_FUNC_CALL or CT_FUNC_DEF
    */
   if (chunk_is_token(next, CT_PAREN_OPEN))
   {
      Chunk *tmp = next->GetNextNcNnl();

      if (  language_is_set(LANG_C | LANG_CPP | LANG_OC)
         && chunk_is_token(tmp, CT_CARET))
      {
         handle_oc_block_type(tmp);

         // This is the case where a block literal is passed as the first argument of a C-style method invocation.
         if (  (  chunk_is_token(tmp, CT_OC_BLOCK_CARET)
               || chunk_is_token(tmp, CT_CARET))
            && chunk_is_token(pc, CT_WORD))
         {
            LOG_FMT(LFCN, "%s(%d): (1) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                    __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
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
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());

         bool is_byref_array = false;

         if (language_is_set(LANG_CPP))
         {
            // If the open paren is followed by an ampersand, an optional word,
            // a close parenthesis, and an open square bracket, then it is an
            // array being passed by reference, not a cast
            tmp = next->GetNextNcNnl();

            if (chunk_is_token(tmp, CT_AMP))
            {
               auto tmp2 = tmp->GetNextNcNnl();

               if (chunk_is_token(tmp2, CT_WORD))
               {
                  tmp2 = tmp2->GetNextNcNnl();
               }

               if (chunk_is_token(tmp2, CT_PAREN_CLOSE))
               {
                  tmp2 = tmp2->GetNextNcNnl();

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
            tmp = next->GetNextType(CT_PAREN_CLOSE, next->level);

            if (tmp->IsNotNullChunk())
            {
               tmp = tmp->GetNext();

               if (chunk_is_token(tmp, CT_PAREN_OPEN))
               {
                  set_chunk_type(pc, CT_FUNCTION);
               }
               else
               {
                  if (  get_chunk_parent_type(pc) == CT_NONE
                     && !pc->flags.test(PCF_IN_TYPEDEF))
                  {
                     tmp = next->GetNextNcNnl();

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
         LOG_FMT(LFCN, "%s(%d): (2) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
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
      return;
   }

   // Mark function parens and braces
   if (  chunk_is_token(pc, CT_FUNC_DEF)
      || chunk_is_token(pc, CT_FUNC_CALL)
      || chunk_is_token(pc, CT_FUNC_CALL_USER)
      || chunk_is_token(pc, CT_FUNC_PROTO))
   {
      Chunk *tmp = next;

      if (chunk_is_token(tmp, CT_SQUARE_OPEN))
      {
         tmp = set_paren_parent(tmp, pc->type);
      }
      else if (  chunk_is_token(tmp, CT_TSQUARE)
              || get_chunk_parent_type(tmp) == CT_OPERATOR)
      {
         tmp = tmp->GetNextNcNnl();
      }

      if (  tmp != nullptr
         && tmp->IsNotNullChunk())
      {
         if (chunk_is_paren_open(tmp))
         {
            tmp = flag_parens(tmp, PCF_NONE, CT_FPAREN_OPEN, pc->type, false);

            if (  tmp != nullptr
               && tmp->IsNotNullChunk())
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
      return;
   }

   // Mark the parameters in catch()
   if (  chunk_is_token(pc, CT_CATCH)
      && chunk_is_token(next, CT_SPAREN_OPEN))
   {
      fix_fcn_def_params(next);
      return;
   }

   if (  chunk_is_token(pc, CT_THROW)
      && chunk_is_token(prev, CT_FPAREN_CLOSE))
   {
      set_chunk_parent(pc, get_chunk_parent_type(prev));

      if (chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_paren_parent(next, CT_THROW);
      }
      return;
   }

   // Mark the braces in: "for_each_entry(xxx) { }"
   if (  chunk_is_token(pc, CT_BRACE_OPEN)
      && get_chunk_parent_type(pc) != CT_DOUBLE_BRACE
      && chunk_is_token(prev, CT_FPAREN_CLOSE)
      && (  get_chunk_parent_type(prev) == CT_FUNC_CALL
         || get_chunk_parent_type(prev) == CT_FUNC_CALL_USER)
      && !pc->flags.test(PCF_IN_CONST_ARGS))
   {
      LOG_FMT(LFCN, "%s(%d): (3) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      set_paren_parent(pc, CT_FUNC_CALL);
      return;
   }

   /*
    * Check for a close parenthesis followed by an open parenthesis,
    * which means that we are on a function type declaration (C/C++ only?).
    * Note that typedefs are already taken care of.
    */
   if (  !pc->flags.test(PCF_IN_TEMPLATE)                         // Issue #3252
      && get_chunk_parent_type(pc) != CT_CPP_CAST
      && get_chunk_parent_type(pc) != CT_C_CAST
      && !pc->flags.test(PCF_IN_PREPROC)
      && !is_oc_block(pc)
      && get_chunk_parent_type(pc) != CT_OC_MSG_DECL
      && get_chunk_parent_type(pc) != CT_OC_MSG_SPEC
      && chunk_is_str(pc, ")")
      && chunk_is_str(next, "("))
   {
      if (language_is_set(LANG_D))
      {
         flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
      }
      else
      {
         mark_function_type(pc);
      }
      return;
   }

   if (chunk_is_token(pc, CT_OC_CLASS))
   {
      handle_oc_class(pc);
      return;
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
         && chunk_is_not_token(prev, CT_DECLTYPE)
         && chunk_is_not_token(prev, CT_SIZEOF)
         && get_chunk_parent_type(prev) != CT_SIZEOF
         && get_chunk_parent_type(prev) != CT_OPERATOR
         && !pc->flags.test(PCF_IN_TYPEDEF))
      {
         fix_casts(pc);
         return;
      }
   }

   if (language_is_set(LANG_CPP))
   {
      Chunk *nnext = next->GetNextNcNnl();

      // handle parent_type of assigns in special functions (ro5 + pure virtual)
      if (  pc->flags.test_any(PCF_IN_STRUCT | PCF_IN_CLASS)
         && chunk_is_token(pc, CT_ASSIGN)
         && chunk_is_token(nnext, CT_SEMICOLON)
         && (  chunk_is_token(next, CT_DEFAULT)
            || chunk_is_token(next, CT_DELETE)
            || (  chunk_is_token(next, CT_NUMBER)
               && chunk_is_str(next, "0"))))
      {
         const size_t level        = pc->level;
         bool         found_status = false;
         Chunk        *pprev       = pc->GetPrev();

         for ( ; (  pprev->IsNotNullChunk()
                 && pprev->level >= level
                 && chunk_is_not_token(pprev, CT_SEMICOLON)
                 && chunk_is_not_token(pprev, CT_ACCESS_COLON))
               ; pprev = pprev->GetPrev())
         {
            if (pprev->level != level)
            {
               continue;
            }

            if (chunk_is_token(next, CT_NUMBER))
            {
               if (  chunk_is_token(pprev, CT_QUALIFIER)
                  && chunk_is_str(pprev, "virtual"))
               {
                  found_status = true;
                  break;
               }
            }
            else
            {
               if (  chunk_is_token(pprev, CT_FUNC_CLASS_PROTO)  // ctor/dtor
                  || chunk_is_token(pprev, CT_FUNC_PROTO))       // normal function
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

      if (detect_cpp_braced_init_list(pc, next))
      {
         flag_cpp_braced_init_list(pc, next);
      }
   }

   // Check for stuff that can only occur at the start of an expression
   if (  pc->flags.test(PCF_EXPR_START)
      || (  prev->flags.test(PCF_EXPR_START)
         && get_chunk_parent_type(pc) == CT_OC_AT))
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

      if (  language_is_set(LANG_CS | LANG_VALA)
         && chunk_is_token(pc, CT_QUESTION)
         && chunk_is_token(prev, CT_ANGLE_CLOSE))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }

      else if (chunk_is_token(pc, CT_MINUS))
      {
         set_chunk_type(pc, CT_NEG);
      }

      else if (chunk_is_token(pc, CT_PLUS))
      {
         set_chunk_type(pc, CT_POS);
      }

      else if (chunk_is_token(pc, CT_INCDEC_AFTER))
      {
         set_chunk_type(pc, CT_INCDEC_BEFORE);
      }

      else if (chunk_is_token(pc, CT_AMP))
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

      else if (chunk_is_token(pc, CT_CARET))
      {
         if (language_is_set(LANG_C | LANG_CPP | LANG_OC))
         {
            // This is likely the start of a block literal
            handle_oc_block_literal(pc);
         }
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

   if (  chunk_is_token(pc, CT_DELETE)
      && chunk_is_token(next, CT_TSQUARE))
   {
      set_chunk_parent(next, CT_DELETE);
   }

   // Change CT_STAR to CT_PTR_TYPE or CT_ARITH or CT_DEREF
   if (  chunk_is_token(pc, CT_STAR)
      || (  language_is_set(LANG_CPP)
         && chunk_is_token(pc, CT_CARET)))
   {
      if (  chunk_is_paren_close(next)
         || chunk_is_token(next, CT_COMMA))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (  language_is_set(LANG_OC)
              && chunk_is_token(next, CT_STAR))
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
                 || get_chunk_parent_type(pc) == CT_SIZEOF))
      {
         set_chunk_type(pc, CT_DEREF);
      }
      else if (  (  chunk_is_token(prev, CT_WORD)
                 && chunk_ends_type(prev)
                 && !prev->flags.test(PCF_IN_FCN_CTOR)
                 && !prev->flags.test(PCF_IN_ARRAY_ASSIGN)) // Issue #3345
              || chunk_is_token(prev, CT_DC_MEMBER)
              || chunk_is_token(prev, CT_PTR_TYPE))
      {
         LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n   ",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
         log_pcf_flags(LFCNR, pc->flags);
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (  chunk_is_token(next, CT_SQUARE_OPEN)
              && !language_is_set(LANG_OC))  // Issue #408
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_token(pc, CT_STAR))
      {
         // Add check for CT_DC_MEMBER CT_WORD CT_STAR sequence
         // to convert CT_WORD into CT_TYPE
         // and CT_STAR into CT_PTR_TYPE
         // look for an assign backward, fuction call, return to distinguish between
         //    double result = Constants::PI * factor;
         // and
         //    ::some::name * foo;
         if (  chunk_is_token(prev, CT_WORD)
            && chunk_is_token(prev->prev, CT_DC_MEMBER)
            && language_is_set(LANG_CPP))
         {
            // Issue 1402
            bool  is_multiplication = false;
            Chunk *tmp              = pc;

            while (tmp->IsNotNullChunk())
            {
               if (  chunk_is_token(tmp, CT_SEMICOLON)
                  || get_chunk_parent_type(tmp) == CT_CLASS)
               {
                  break;
               }
               else if (  chunk_is_token(tmp, CT_ASSIGN)
                       || chunk_is_token(tmp, CT_FUNC_CALL)
                       || chunk_is_token(tmp, CT_RETURN))
               {
                  is_multiplication = true;
                  break;
               }
               tmp = tmp->GetPrevNcNnlNi(); // Issue #2279
            }

            if (is_multiplication)
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
                    && (  chunk_is_opening_brace(next)
                       || pc->GetNext()->IsStar()))
                 || chunk_is_token(next, CT_QUALIFIER))               // Issue #2648
         {
            // example:
            // auto getComponent(Color *color) -> Component * {
            // auto getComponent(Color *color) -> Component ** {
            // auto getComponent(Color *color) -> Component * _Nonnull
            // only to help the vim command }}
            set_chunk_type(pc, CT_PTR_TYPE);
         }
         else if (  chunk_is_token(pc->next, CT_SEMICOLON)      // Issue #2319
                 || (  chunk_is_token(pc->next, CT_STAR)
                    && chunk_is_token(pc->next->next, CT_STAR)))
         {
            // more pointers are NOT yet possible
            fprintf(stderr, "Too many pointers: the maximum level of pointer indirection is 3 (i.e., ***p)\n");
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
                           && chunk_is_not_token(prev, CT_SQUARE_CLOSE)
                           && chunk_is_not_token(prev, CT_DC_MEMBER)) ? CT_DEREF : CT_ARITH);
         }

         if (pc->flags.test(PCF_IN_TYPEDEF))  // Issue #1255/#633
         {
            Chunk *tmp = pc;

            while (tmp->IsNotNullChunk())
            {
               if (  chunk_is_token(tmp, CT_SEMICOLON)
                  || chunk_is_token(tmp, CT_BRACE_OPEN)
                  || chunk_is_token(tmp, CT_SQUARE_OPEN)) // Issue #3342
               {
                  break;
               }
               else if (chunk_is_token(tmp, CT_TYPEDEF))
               {
                  set_chunk_type(pc, CT_PTR_TYPE);
               }
               tmp = tmp->GetPrevNcNnlNi(); // Issue #2279
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
      else if (  chunk_is_token(prev, CT_TYPE)
              || chunk_is_token(prev, CT_QUALIFIER))
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else if (  chunk_is_token(prev, CT_WORD)             // Issue #3204
              && chunk_is_token(next, CT_OPERATOR))
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else if (  chunk_is_token(next, CT_FPAREN_CLOSE)
              || chunk_is_token(next, CT_COMMA))
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

            if (  chunk_is_token(prev, CT_WORD)
               && !chunk_is_token(next, CT_NUMBER)) // Issue #3407
            {
               Chunk *tmp = prev->GetPrevNcNnlNi(); // Issue #2279

               if (tmp->IsNotNullChunk())
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
                        LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', set PCF_VAR_1ST\n",
                                __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
                        chunk_flags_set(next, PCF_VAR_1ST);
                     }
                  }
                  else if (chunk_is_token(tmp, CT_DC_MEMBER))
                  {
                     set_chunk_type(prev, CT_TYPE);
                  }
               }
            }
         }
      }
   }

   if (  chunk_is_token(pc, CT_MINUS)
      || chunk_is_token(pc, CT_PLUS))
   {
      if (  chunk_is_token(prev, CT_POS)
         || chunk_is_token(prev, CT_NEG)
         || chunk_is_token(prev, CT_ARITH)
         || chunk_is_token(prev, CT_SHIFT))
      {
         set_chunk_type(pc, chunk_is_token(pc, CT_MINUS) ? CT_NEG : CT_POS);
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
                  if (unc_text::compare(pc->prev->Text(), "\"C\"") == 0)
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
               Chunk *tmp = pc->GetPrevType(CT_FPAREN_OPEN, pc->level - 1);

               if (  tmp->IsNotNullChunk()
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
            Chunk *tmp = pc;

            while (  tmp != nullptr
                  && tmp->IsNotNullChunk())
            {
               if (chunk_is_token(tmp, CT_ATTRIBUTE))
               {
                  LOG_FMT(LFCNR, "%s(%d): ATTRIBUTE found, type is %s, Text() '%s'\n",
                          __func__, __LINE__, get_token_name(tmp->type), tmp->Text());
                  LOG_FMT(LFCNR, "for token, type is %s, Text() '%s'\n", get_token_name(pc->type), pc->Text());
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
               tmp = tmp->GetPrev();
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

      bool       is_preproc = pc->flags.test(PCF_IN_PREPROC);

      auto const search_assign = [&pc, &is_preproc]()
      {
         for (Chunk *temp = pc; temp->IsNotNullChunk(); temp = temp->GetNextNcNnl())
         {
            LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, temp->orig_line, temp->orig_col,
                    temp->Text(), get_token_name(temp->type));

            if (chunk_is_token(temp, CT_ASSIGN))
            {
               return(true);
            }

            if (  chunk_is_token(temp, CT_SEMICOLON)
               || (  is_preproc
                  && (  !temp->flags.test(PCF_IN_PREPROC)
                     || chunk_is_token(temp, CT_PREPROC))))
            {
               return(false);
            }
         }

         return(false);
      };

      const bool assign_found = language_is_set(LANG_D) || search_assign();

      if (assign_found)
      {
         // it is a Type alias, alias template
         for (Chunk *temp = pc; temp->IsNotNullChunk(); temp = temp->GetNextNcNnl())
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
   if (  chunk_is_token(pc, CT_BOOL)
      && !pc->flags.test(PCF_IN_PREPROC)
      && chunk_is_str(pc, "&&")
      && chunk_ends_type(pc->prev))
   {
      Chunk *tmp = pc->GetPrev();                 // Issue #2688
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col,
              tmp->Text(), get_token_name(tmp->type));
      log_pcf_flags(LFCNR, tmp->flags);
      // look for a type

      if (chunk_is_token(tmp, CT_TYPE))
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col,
                 pc->Text(), get_token_name(pc->type));
         log_pcf_flags(LFCNR, pc->flags);
         set_chunk_type(pc, CT_BYREF);
      }
      // look next, is there a "assign" before the ";"
      Chunk *semi = pc->GetNextType(CT_SEMICOLON, pc->level);                // Issue #2688

      if (semi->IsNotNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, semi->orig_line, semi->orig_col,
                 semi->Text(), get_token_name(semi->type));

         for (Chunk *test_it = pc; test_it != semi; test_it = test_it->GetNext())
         {
            LOG_FMT(LFCNR, "%s(%d): test_it->orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
                    __func__, __LINE__, test_it->orig_line, test_it->orig_col,
                    test_it->Text(), get_token_name(test_it->type));

            if (chunk_is_token(test_it, CT_ASSIGN))
            {
               // the statement is an assigment
               // && is before assign
               set_chunk_type(pc, CT_BYREF);
               break;
            }
         }
      }
   }

   // Issue #1704
   if (  chunk_is_token(pc, CT_INCDEC_AFTER)
      && pc->flags.test(PCF_IN_PREPROC))
   {
      Chunk *tmp_2 = pc->GetNext();
      LOG_FMT(LFCNR, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col,
              pc->Text(), get_token_name(pc->type));
      log_pcf_flags(LFTYPE, pc->flags);

      if (chunk_is_token(tmp_2, CT_WORD))
      {
         set_chunk_type(pc, CT_INCDEC_BEFORE);
      }
   }
} // do_symbol_check


static void check_double_brace_init(Chunk *bo1)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LJDBI, "%s(%d): orig_line is %zu, orig_col is %zu", __func__, __LINE__, bo1->orig_line, bo1->orig_col);
   Chunk *pc = bo1->GetPrevNcNnlNi();   // Issue #2279

   if (pc->IsNullChunk())
   {
      return;
   }

   if (chunk_is_paren_close(pc))
   {
      Chunk *bo2 = bo1->GetNext();

      if (bo2->IsNullChunk())
      {
         return;
      }

      if (chunk_is_token(bo2, CT_BRACE_OPEN))
      {
         // found a potential double brace
         Chunk *bc2 = chunk_skip_to_match(bo2);

         if (bc2 == nullptr)
         {
            return;
         }
         Chunk *bc1 = bc2->GetNext();

         if (bc1->IsNullChunk())
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
   Chunk *pc;
   Chunk dummy;

   cpd.unc_stage = unc_stage_e::FIX_SYMBOLS;

   mark_define_expressions();

   bool is_cpp  = language_is_set(LANG_CPP);
   bool is_java = language_is_set(LANG_JAVA);

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
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
      // a brace immediately preceded by word in C++11 is an initializer list though it may also
      // by a type casting initializer list if the word is really a type; sadly uncrustify knows
      // only built-in types and knows nothing of user-defined types
      Chunk *prev = pc->GetPrevNcNnlNi();   // Issue #2279

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
         Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

         if (  next->IsNotNullChunk()
            && chunk_is_token(next, CT_PAREN_OPEN))
         {
            flag_parens(next, PCF_NONE, CT_FPAREN_OPEN, CT_ATTRIBUTE, false);
         }
      }
   }

   pc = Chunk::GetHead();

   if (pc->IsCommentOrNewline())
   {
      pc = pc->GetNextNcNnl();
   }

   while (pc->IsNotNullChunk())
   {
      if (chunk_is_token(pc, CT_IGNORED))
      {
         pc = pc->GetNextNcNnl();
         continue;
      }
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line       is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
      Chunk *prev = pc->GetPrevNcNnlNi(E_Scope::PREPROC);   // Issue #2279

      if (chunk_is_token(prev, CT_QUALIFIER))
      {
         prev = prev->GetPrevNcNnlNi(E_Scope::PREPROC);     // Issue #3513
      }

      if (prev->IsNullChunk())
      {
         prev = &dummy;
      }
      else
      {
         // Issue #2279
         LOG_FMT(LFCNR, "%s(%d): prev(ni)->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, prev->orig_line, prev->orig_col, prev->Text(), get_token_name(prev->type));
      }
      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

      if (next->IsNullChunk())
      {
         next = &dummy;
      }
      else
      {
         // Issue #2279
         LOG_FMT(LFCNR, "%s(%d): next->orig_line     is %zu, orig_col is %zu, Text() is '%s', type is %s\n",
                 __func__, __LINE__, next->orig_line, next->orig_col, next->Text(), get_token_name(next->type));
      }
      LOG_FMT(LFCNR, "%s(%d): do_symbol_check(%s, %s, %s)\n",
              __func__, __LINE__, prev->Text(), pc->Text(), next->Text());
      do_symbol_check(prev, pc, next);
      pc = pc->GetNextNcNnl();
   }
   pawn_add_virtual_semicolons();
   process_returns();

   /*
    * 2nd pass - handle variable definitions
    * REVISIT: We need function params marked to do this (?)
    */
   pc = Chunk::GetHead();
   int square_level = -1;

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      char copy[1000];
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s, parent_type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->ElidedText(copy), get_token_name(pc->type), get_token_name(pc->parent_type));

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
         Chunk *next = pc->GetNextNcNnl();

         if (chunk_is_token(next, CT_STRING))
         {
            Chunk *tmp = next->GetNextNcNnl();

            while (tmp->IsNotNullChunk())
            {
               if (  chunk_is_token(tmp, CT_TYPE)
                  || chunk_is_token(tmp, CT_BRACE_OPEN)
                  || chunk_is_token(tmp, CT_ATTRIBUTE))
               {
                  break;
               }

               if (chunk_is_token(tmp, CT_WORD))
               {
                  chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
                  break;
               }
               tmp = tmp->GetNextNcNnl();
            }
         }
      }

      if (  chunk_is_token(pc, CT_ATTRIBUTE)
         && language_is_set(LANG_ALLC))
      {
         Chunk *tmp = skip_attribute_next(pc);

         if (chunk_is_token(tmp, CT_WORD))
         {
            chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
         }
      }

      if (  chunk_is_token(pc, CT_BRACE_OPEN)                       // Issue #2332
         && get_chunk_parent_type(pc) == CT_BRACED_INIT_LIST)
      {
         LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', look for CT_BRACE_OPEN\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
         pc = pc->GetNextType(CT_BRACE_CLOSE, pc->level);
      }
      /*
       * A variable definition is possible after at the start of a statement
       * that starts with: DC_MEMBER, QUALIFIER, TYPE, or WORD
       */
      // Issue #2279
      // Issue #2478
      LOG_FMT(LFCNR, "%s(%d): pc->orig_line is %zu, orig_col is %zu, Text() is '%s', type is %s, parent_type is %s\n   ",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->ElidedText(copy), get_token_name(pc->type), get_token_name(pc->parent_type));
      log_pcf_flags(LFCNR, pc->flags);

      if (  (square_level < 0)
         && pc->flags.test(PCF_STMT_START)
         && (  chunk_is_token(pc, CT_QUALIFIER)
            || chunk_is_token(pc, CT_TYPE)
            || chunk_is_token(pc, CT_TYPENAME)
            || chunk_is_token(pc, CT_DC_MEMBER)                         // Issue #2478
            || chunk_is_token(pc, CT_WORD))
         && get_chunk_parent_type(pc) != CT_BIT_COLON
         && get_chunk_parent_type(pc) != CT_ENUM
         && !pc->flags.test(PCF_IN_CLASS_BASE)
         && !pc->flags.test(PCF_IN_ENUM))
      {
         pc = fix_variable_definition(pc);
      }
      else
      {
         pc = pc->GetNextNcNnl();
      }
   }
} // fix_symbols


static void process_returns(void)
{
   LOG_FUNC_ENTRY();
   Chunk *pc;

   pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (chunk_is_not_token(pc, CT_RETURN))
      {
         pc = pc->GetNextType(CT_RETURN, -1);
         continue;
      }
      pc = process_return(pc);
   }
}


static Chunk *process_return(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *next;
   Chunk *temp;
   Chunk *semi;
   Chunk *cpar;
   Chunk chunk;

   // grab next and bail if it is a semicolon
   next = pc->PpaGetNextNcNnl();

   if (  next->IsNullChunk()
      || chunk_is_semicolon(next)
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
      cpar = next->GetNextType(CT_PAREN_CLOSE, next->level);

      if (cpar->IsNullChunk())
      {
         return(Chunk::NullChunkPtr);
      }
      semi = cpar->PpaGetNextNcNnl();

      if (semi->IsNullChunk())
      {
         return(Chunk::NullChunkPtr);
      }

      if (  chunk_is_token(semi, CT_NEWLINE)
         || chunk_is_semicolon(semi))
      {
         log_rule_B("mod_paren_on_return");

         if (options::mod_paren_on_return() == IARF_REMOVE)
         {
            LOG_FMT(LRETURN, "%s(%d): removing parens on orig_line %zu\n",
                    __func__, __LINE__, pc->orig_line);

            // lower the level of everything
            for (temp = next; temp != cpar; temp = temp->GetNext())
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

            while (  temp->IsNotNullChunk()
                  && chunk_is_not_token(temp, CT_NEWLINE))
            {
               temp->column       = temp->column - 2;
               temp->orig_col     = temp->orig_col - 2;
               temp->orig_col_end = temp->orig_col_end - 2;
               temp               = temp->GetNext();
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
   if (  language_is_set(LANG_CPP)
      && chunk_is_token(next, CT_BRACE_OPEN)
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
      while ((semi = semi->GetNext())->IsNotNullChunk())
      {
         if (!semi->flags.test(PCF_IN_PREPROC))
         {
            break;
         }

         if (semi->level < pc->level)
         {
            return(semi);
         }

         if (  chunk_is_semicolon(semi)
            && pc->level == semi->level)
         {
            break;
         }
      }
   }
   else
   {
      while ((semi = semi->GetNext())->IsNotNullChunk())
      {
         if (semi->level < pc->level)
         {
            return(semi);
         }

         if (  chunk_is_semicolon(semi)
            && pc->level == semi->level)
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
      chunk.pp_level    = pc->pp_level;
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

      for (temp = next; temp != cpar; temp = temp->GetNext())
      {
         temp->level++;
      }
   }
   return(semi);
} // process_return


static bool is_oc_block(Chunk *pc)
{
   return(  pc != nullptr
         && (  get_chunk_parent_type(pc) == CT_OC_BLOCK_TYPE
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_EXPR
            || get_chunk_parent_type(pc) == CT_OC_BLOCK_ARG
            || get_chunk_parent_type(pc) == CT_OC_BLOCK
            || chunk_is_token(pc, CT_OC_BLOCK_CARET)
            || (  pc->next != nullptr
               && pc->next->type == CT_OC_BLOCK_CARET)
            || (  pc->prev != nullptr
               && pc->prev->type == CT_OC_BLOCK_CARET)));
}


void mark_comments(void)
{
   LOG_FUNC_ENTRY();

   cpd.unc_stage = unc_stage_e::MARK_COMMENTS;

   bool  prev_nl = true;
   Chunk *cur    = Chunk::GetHead();

   while (cur->IsNotNullChunk())
   {
      Chunk *next   = cur->GetNextNvb();
      bool  next_nl = next->IsNullChunk() || chunk_is_newline(next);

      if (cur->IsComment())
      {
         if (  next_nl
            && prev_nl)
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


static void handle_cpp_template(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *tmp = pc->GetNextNcNnl();

   if (chunk_is_not_token(tmp, CT_ANGLE_OPEN))
   {
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);

   size_t level = tmp->level;

   tmp = tmp->GetNext();

   while (tmp->IsNotNullChunk())
   {
      if (  chunk_is_token(tmp, CT_CLASS)
         || chunk_is_token(tmp, CT_STRUCT))
      {
         set_chunk_type(tmp, CT_TYPE);
      }
      else if (  chunk_is_token(tmp, CT_ANGLE_CLOSE)
              && tmp->level == level)
      {
         set_chunk_parent(tmp, CT_TEMPLATE);
         break;
      }
      tmp = tmp->GetNext();
   }

   if (tmp->IsNotNullChunk())
   {
      tmp = tmp->GetNextNcNnl();

      if (chunk_is_token(tmp, CT_FRIEND))
      {
         // Account for a template friend declaration
         set_chunk_parent(tmp, CT_TEMPLATE);

         tmp = tmp->GetNextNcNnl();
      }

      if (  chunk_is_token(tmp, CT_CLASS)
         || chunk_is_token(tmp, CT_STRUCT))
      {
         set_chunk_parent(tmp, CT_TEMPLATE);

         // REVISIT: This may be a bit risky - might need to track the { };
         tmp = tmp->GetNextType(CT_SEMICOLON, tmp->level);

         if (tmp->IsNotNullChunk())
         {
            set_chunk_parent(tmp, CT_TEMPLATE);
         }
      }
   }
} // handle_cpp_template


static void handle_cpp_lambda(Chunk *sq_o)
{
   LOG_FUNC_ENTRY();

   Chunk *ret = Chunk::NullChunkPtr;

   // abort if type of the previous token is not contained in this whitelist
   Chunk *prev = sq_o->GetPrevNcNnlNi();   // Issue #2279

   if (prev->IsNullChunk())
   {
      LOG_FMT(LFCNR, "%s(%d): prev is nullptr\n", __func__, __LINE__);
   }

   if (  prev->IsNullChunk()
      || (  chunk_is_not_token(prev, CT_ASSIGN)
         && chunk_is_not_token(prev, CT_COMMA)
         && chunk_is_not_token(prev, CT_PAREN_OPEN)   // allow Js like self invoking lambda syntax: ([](){})();
         && chunk_is_not_token(prev, CT_FPAREN_OPEN)
         && chunk_is_not_token(prev, CT_SQUARE_OPEN)
         && chunk_is_not_token(prev, CT_BRACE_OPEN)
         && chunk_is_not_token(prev, CT_SEMICOLON)
         && chunk_is_not_token(prev, CT_RETURN)))
   {
      LOG_FMT(LFCNR, "%s(%d): return\n", __func__, __LINE__);
      return;
   }
   Chunk *sq_c = sq_o; // assuming '[]'

   if (chunk_is_token(sq_o, CT_SQUARE_OPEN))
   {
      // make sure there is a ']'
      sq_c = chunk_skip_to_match(sq_o);

      if (sq_c == nullptr)
      {
         LOG_FMT(LFCNR, "%s(%d): return\n", __func__, __LINE__);
         return;
      }
   }
   Chunk *pa_o = sq_c->GetNextNcNnl();

   // check to see if there is a lambda-specifier in the pa_o chunk;
   // assuming chunk is CT_EXECUTION_CONTEXT, ignore lambda-specifier
   while (chunk_is_token(pa_o, CT_EXECUTION_CONTEXT))
   {
      // set pa_o to next chunk after this specifier
      pa_o = pa_o->GetNextNcNnl();
   }

   if (pa_o->IsNullChunk())
   {
      LOG_FMT(LFCNR, "%s(%d): return\n", __func__, __LINE__);
      return;
   }
   Chunk *pa_c = Chunk::NullChunkPtr;

   // lambda-declarator '( params )' is optional
   if (chunk_is_token(pa_o, CT_PAREN_OPEN))
   {
      // and now find the ')'
      pa_c = chunk_skip_to_match(pa_o);

      if (pa_c->IsNullChunk())
      {
         LOG_FMT(LFCNR, "%s(%d): return\n", __func__, __LINE__);
         return;
      }
   }
   // Check for 'mutable' keyword: '[]() mutable {}' or []() mutable -> ret {}
   Chunk *br_o = pa_c->IsNotNullChunk() ? pa_c->GetNextNcNnl() : pa_o;

   if (chunk_is_str(br_o, "mutable"))
   {
      br_o = br_o->GetNextNcNnl();
   }
   //TODO: also check for exception and attribute between [] ... {}

   // skip possible arrow syntax: '-> ret'
   if (chunk_is_str(br_o, "->"))
   {
      ret = br_o;
      // REVISIT: really should check the stuff we are skipping
      br_o = br_o->GetNextType(CT_BRACE_OPEN, br_o->level);
   }

   // skip possible CT_NOEXCEPT
   if (chunk_is_token(br_o, CT_NOEXCEPT)) // Issue #3321
   {
      ret = br_o;
      // REVISIT: really should check the stuff we are skipping
      br_o = br_o->GetNextType(CT_BRACE_OPEN, br_o->level);
   }

   if (br_o->IsNullChunk())
   {
      LOG_FMT(LFCNR, "%s(%d): br_o is null. Return\n", __func__, __LINE__);
      return;
   }

   if (chunk_is_not_token(br_o, CT_BRACE_OPEN))
   {
      LOG_FMT(LFCNR, "%s(%d): br_o is '%s'/%s\n",
              __func__, __LINE__,
              br_o->Text(), get_token_name(br_o->type));
      LOG_FMT(LFCNR, "%s(%d): return\n", __func__, __LINE__);
      return;
   }
   // and now find the '}'
   Chunk *br_c = chunk_skip_to_match(br_o);

   if (br_c->IsNullChunk())
   {
      LOG_FMT(LFCNR, "%s(%d): return\n", __func__, __LINE__);
      return;
   }

   // This looks like a lambda expression
   if (chunk_is_token(sq_o, CT_TSQUARE))
   {
      // split into two chunks
      Chunk nc;

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

   if (pa_c->IsNotNullChunk())
   {
      set_chunk_type(pa_o, CT_LPAREN_OPEN);                    // Issue #3054
      set_chunk_parent(pa_o, CT_CPP_LAMBDA);
      chunk_set_parent(pa_o, sq_o);
      chunk_set_parent(br_o, sq_o);
      set_chunk_type(pa_c, CT_LPAREN_CLOSE);
      set_chunk_parent(pa_c, CT_CPP_LAMBDA);
      chunk_set_parent(pa_c, sq_o);
      chunk_set_parent(br_c, sq_o);
   }
   set_chunk_parent(br_o, CT_CPP_LAMBDA);
   set_chunk_parent(br_c, CT_CPP_LAMBDA);

   if (ret->IsNotNullChunk())
   {
      set_chunk_type(ret, CT_CPP_LAMBDA_RET);
      ret = ret->GetNextNcNnl();

      while (ret != br_o)
      {
         make_type(ret);
         ret = ret->GetNextNcNnl();
      }
   }

   if (pa_c->IsNotNullChunk())
   {
      fix_fcn_def_params(pa_o);
   }
   //handle self calling lambda paren
   Chunk *call_pa_o = br_c->GetNextNcNnl();

   if (chunk_is_token(call_pa_o, CT_PAREN_OPEN))
   {
      Chunk *call_pa_c = chunk_skip_to_match(call_pa_o);

      if (call_pa_c->IsNotNullChunk())
      {
         set_chunk_type(call_pa_o, CT_FPAREN_OPEN);
         set_chunk_parent(call_pa_o, CT_FUNC_CALL);
         set_chunk_type(call_pa_c, CT_FPAREN_CLOSE);
         set_chunk_parent(call_pa_c, CT_FUNC_CALL);
      }
   }
   mark_cpp_lambda(sq_o);
} // handle_cpp_lambda


static void handle_d_template(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *name = pc->GetNextNcNnl();
   Chunk *po   = name->GetNextNcNnl();

   //if (!name || (name->type != CT_WORD && name->type != CT_WORD))  Coverity CID 76000 Same on both sides, 2016-03-16
   if (  name->IsNullChunk()
      || chunk_is_not_token(name, CT_WORD))
   {
      // TODO: log an error, expected NAME
      return;
   }

   if (  po->IsNullChunk()
      || chunk_is_not_token(po, CT_PAREN_OPEN))
   {
      // TODO: log an error, expected '('
      return;
   }
   set_chunk_type(name, CT_TYPE);
   set_chunk_parent(name, CT_TEMPLATE);
   set_chunk_parent(po, CT_TEMPLATE);

   ChunkStack cs;
   Chunk      *tmp = get_d_template_types(cs, po);

   if (  tmp == nullptr
      || chunk_is_not_token(tmp, CT_PAREN_CLOSE))
   {
      // TODO: log an error, expected ')'
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);

   tmp = tmp->GetNextNcNnl();

   if (chunk_is_not_token(tmp, CT_BRACE_OPEN))
   {
      // TODO: log an error, expected '{'
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);
   po  = tmp;
   tmp = tmp->GetNextNcNnl();

   while (  tmp->IsNotNullChunk()
         && tmp->level > po->level)
   {
      if (  chunk_is_token(tmp, CT_WORD)
         && chunkstack_match(cs, tmp))
      {
         set_chunk_type(tmp, CT_TYPE);
      }
      tmp = tmp->GetNextNcNnl();
   }
//   if (!chunk_is_token(tmp, CT_BRACE_CLOSE))
//   {
//      // TODO: log an error, expected '}'
//   }
   set_chunk_parent(tmp, CT_TEMPLATE);
} // handle_d_template


Chunk *skip_template_next(Chunk *ang_open)
{
   if (ang_open == nullptr)
   {
      return(Chunk::NullChunkPtr);
   }

   if (chunk_is_token(ang_open, CT_ANGLE_OPEN))
   {
      Chunk *pc = ang_open->GetNextType(CT_ANGLE_CLOSE, ang_open->level);

      if (pc->IsNullChunk())
      {
         return(Chunk::NullChunkPtr);
      }
      return(pc->GetNextNcNnl());
   }
   return(ang_open);
}


static void handle_oc_class(Chunk *pc)
{
   enum class angle_state_e : unsigned int
   {
      NONE  = 0,
      OPEN  = 1, // '<' found
      CLOSE = 2, // '>' found
   };

   LOG_FUNC_ENTRY();
   Chunk         *tmp;
   bool          hit_scope     = false;
   bool          passed_name   = false; // Did we pass the name of the class and now there can be only protocols, not generics
   int           generic_level = 0;     // level of depth of generic
   angle_state_e as            = angle_state_e::NONE;

   LOG_FMT(LOCCLASS, "%s(%d): start [%s] [%s] line %zu\n",
           __func__, __LINE__, pc->Text(), get_token_name(get_chunk_parent_type(pc)), pc->orig_line);

   if (get_chunk_parent_type(pc) == CT_OC_PROTOCOL)
   {
      tmp = pc->GetNextNcNnl();

      if (chunk_is_semicolon(tmp))
      {
         set_chunk_parent(tmp, get_chunk_parent_type(pc));
         LOG_FMT(LOCCLASS, "%s(%d):   bail on semicolon\n", __func__, __LINE__);
         return;
      }
   }
   tmp = pc;

   while (  (tmp = tmp->GetNextNnl()) != nullptr
         && tmp->IsNotNullChunk())
   {
      LOG_FMT(LOCCLASS, "%s(%d):       orig_line is %zu, [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->Text());

      if (chunk_is_token(tmp, CT_OC_END))
      {
         break;
      }

      if (chunk_is_token(tmp, CT_PAREN_OPEN))
      {
         passed_name = true;
      }

      if (chunk_is_str(tmp, "<"))
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

      if (chunk_is_str(tmp, ">"))
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

      if (chunk_is_str(tmp, ">>"))
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
         tmp = tmp->GetNextType(CT_BRACE_CLOSE, tmp->level);

         if (  tmp->IsNotNullChunk()
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
      else if (  chunk_is_str(tmp, "-")
              || chunk_is_str(tmp, "+"))
      {
         as = angle_state_e::CLOSE;

         if (chunk_is_newline(tmp->GetPrev()))
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
      tmp = tmp->GetNextType(CT_BRACE_CLOSE, tmp->level);

      if (tmp->IsNotNullChunk())
      {
         set_chunk_parent(tmp, CT_OC_CLASS);
      }
   }
} // handle_oc_class


static void handle_oc_block_literal(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      return; // let's be paranoid
   }
   Chunk *prev = pc->GetPrevNcNnlNi();   // Issue #2279
   Chunk *next = pc->GetNextNcNnl();

   if (  prev->IsNullChunk()
      || next->IsNullChunk())
   {
      return; // let's be paranoid
   }
   /*
    * block literal: '^ RTYPE ( ARGS ) { }'
    * RTYPE and ARGS are optional
    */
   LOG_FMT(LOCBLK, "%s(%d): block literal @ orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, pc->orig_line, pc->orig_col);

   Chunk *apo = Chunk::NullChunkPtr; // arg paren open
   Chunk *bbo = Chunk::NullChunkPtr; // block brace open
   Chunk *bbc;                       // block brace close

   LOG_FMT(LOCBLK, "%s(%d):  + scan", __func__, __LINE__);
   Chunk *tmp;

   for (tmp = next; tmp->IsNotNullChunk(); tmp = tmp->GetNextNcNnl())
   {
      /* handle '< protocol >' */
      if (chunk_is_str(tmp, "<"))
      {
         Chunk *ao = tmp;
         Chunk *ac = ao->GetNextString(">", 1, ao->level);

         if (ac->IsNotNullChunk())
         {
            set_chunk_type(ao, CT_ANGLE_OPEN);
            set_chunk_parent(ao, CT_OC_PROTO_LIST);
            set_chunk_type(ac, CT_ANGLE_CLOSE);
            set_chunk_parent(ac, CT_OC_PROTO_LIST);

            for (tmp = ao->GetNext(); tmp != ac; tmp = tmp->GetNext())
            {
               tmp->level += 1;
               set_chunk_parent(tmp, CT_OC_PROTO_LIST);
            }

            tmp = ac->GetNextNcNnl();
         }
         else
         {
            tmp = Chunk::NullChunkPtr;
         }
      }
      LOG_FMT(LOCBLK, " '%s'", tmp->Text());

      if (  tmp->level < pc->level
         || chunk_is_token(tmp, CT_SEMICOLON))
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

   if (  bbo->IsNullChunk()
      || bbc == nullptr
      || bbc->IsNullChunk())
   {
      LOG_FMT(LOCBLK, " -- no braces found\n");
      return;
   }
   LOG_FMT(LOCBLK, "\n");

   // we are on a block literal for sure
   set_chunk_type(pc, CT_OC_BLOCK_CARET);
   set_chunk_parent(pc, CT_OC_BLOCK_EXPR);

   // handle the optional args
   Chunk *lbp; // last before paren - end of return type, if any

   if (apo->IsNotNullChunk())
   {
      Chunk *apc = chunk_skip_to_match(apo);  // arg parenthesis close

      if (chunk_is_paren_close(apc))
      {
         LOG_FMT(LOCBLK, " -- marking parens @ apo->orig_line is %zu, apo->orig_col is %zu and apc->orig_line is %zu, apc->orig_col is %zu\n",
                 apo->orig_line, apo->orig_col, apc->orig_line, apc->orig_col);
         flag_parens(apo, PCF_OC_ATYPE, CT_FPAREN_OPEN, CT_OC_BLOCK_EXPR, true);
         fix_fcn_def_params(apo);
      }
      lbp = apo->GetPrevNcNnlNi();   // Issue #2279
   }
   else
   {
      lbp = bbo->GetPrevNcNnlNi();   // Issue #2279
   }

   // mark the return type, if any
   while (lbp != pc)
   {
      LOG_FMT(LOCBLK, " -- lbp %s[%s]\n", lbp->Text(), get_token_name(lbp->type));
      make_type(lbp);
      chunk_flags_set(lbp, PCF_OC_RTYPE);
      set_chunk_parent(lbp, CT_OC_BLOCK_EXPR);
      lbp = lbp->GetPrevNcNnlNi();   // Issue #2279
   }
   // mark the braces
   set_chunk_parent(bbo, CT_OC_BLOCK_EXPR);
   set_chunk_parent(bbc, CT_OC_BLOCK_EXPR);
} // handle_oc_block_literal


static void handle_oc_block_type(Chunk *pc)
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
   Chunk *tpo = pc->GetPrevNcNnlNi(); // type paren open   Issue #2279

   if (chunk_is_paren_open(tpo))
   {
      /*
       * block type: 'RTYPE (^LABEL)(ARGS)'
       * LABEL is optional.
       */
      Chunk *tpc = chunk_skip_to_match(tpo); // type close paren (after '^')
      Chunk *nam = tpc->GetPrevNcNnlNi();    // name (if any) or '^'   Issue #2279
      Chunk *apo = tpc->GetNextNcNnl();      // arg open paren
      Chunk *apc = chunk_skip_to_match(apo); // arg close paren

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
         Chunk   *aft = apc->GetNextNcNnl();
         E_Token pt;

         if (chunk_is_str(nam, "^"))
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
         LOG_FMT(LOCBLK, "%s(%d): block type @ orig_line is %zu, orig_col is %zu, Text() '%s'[%s]\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, nam->Text(), get_token_name(nam->type));
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
         mark_function_return_type(nam, tpo->GetPrevNcNnlNi(), pt);   // Issue #2279
      }
   }
} // handle_oc_block_type


static Chunk *handle_oc_md_type(Chunk *paren_open, E_Token ptype, pcf_flags_t flags, bool &did_it)
{
   Chunk *paren_close;

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

   for (Chunk *cur = paren_open->GetNextNcNnl();
        cur != paren_close;
        cur = cur->GetNextNcNnl())
   {
      LOG_FMT(LOCMSGD, " <%s|%s>", cur->Text(), get_token_name(cur->type));
      chunk_flags_set(cur, flags);
      make_type(cur);
   }

   // returning the chunk after the paren close
   return(paren_close->GetNextNcNnl());
}


static void handle_oc_message_decl(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   bool did_it;
   //bool      in_paren  = false;
   //int       paren_cnt = 0;
   //int       arg_cnt   = 0;

   // Figure out if this is a spec or decl
   Chunk *tmp = pc;

   if (tmp == nullptr)
   {
      tmp = Chunk::NullChunkPtr;
   }

   while ((tmp = tmp->GetNext())->IsNotNullChunk())
   {
      if (tmp->level < pc->level)
      {
         // should not happen
         return;
      }

      if (  chunk_is_token(tmp, CT_SEMICOLON)
         || chunk_is_token(tmp, CT_BRACE_OPEN))
      {
         break;
      }
   }

   if (tmp == nullptr)
   {
      return;
   }
   E_Token pt = chunk_is_token(tmp, CT_SEMICOLON) ? CT_OC_MSG_SPEC : CT_OC_MSG_DECL;

   set_chunk_type(pc, CT_OC_SCOPE);
   set_chunk_parent(pc, pt);

   LOG_FMT(LOCMSGD, "%s(%d): %s @ orig_line is %zu, orig_col is %zu -",
           __func__, __LINE__, get_token_name(pt), pc->orig_line, pc->orig_col);

   // format: -(TYPE) NAME [: (TYPE)NAME

   // handle the return type
   tmp = handle_oc_md_type(pc->GetNextNcNnl(), pt, PCF_OC_RTYPE, did_it);

   if (!did_it)
   {
      LOG_FMT(LOCMSGD, " -- missing type parens\n");
      return;
   }

   // expect the method name/label
   if (chunk_is_not_token(tmp, CT_WORD))
   {
      LOG_FMT(LOCMSGD, " -- missing method name\n");
      return;
   }  // expect the method name/label

   Chunk *label = tmp;

   set_chunk_type(tmp, pt);
   set_chunk_parent(tmp, pt);
   pc = tmp->GetNextNcNnl();

   LOG_FMT(LOCMSGD, " [%s]%s", pc->Text(), get_token_name(pc->type));

   // if we have a colon next, we have args
   if (  chunk_is_token(pc, CT_COLON)
      || chunk_is_token(pc, CT_OC_COLON))
   {
      pc = label;

      while (true)
      {
         // skip optional label
         if (  chunk_is_token(pc, CT_WORD)
            || chunk_is_token(pc, pt))
         {
            set_chunk_parent(pc, pt);
            pc = pc->GetNextNcNnl();
         }

         // a colon must be next
         if (!chunk_is_str(pc, ":"))
         {
            break;
         }
         set_chunk_type(pc, CT_OC_COLON);
         set_chunk_parent(pc, pt);
         pc = pc->GetNextNcNnl();

         // next is the type in parens
         LOG_FMT(LOCMSGD, "  (%s)", pc->Text());
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
         LOG_FMT(LOCMSGD, " arg[%s]", pc->Text());
         pc = pc->GetNextNcNnl();
      }
   }
   LOG_FMT(LOCMSGD, " end[%s]", pc->Text());

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


static void handle_oc_message_send(Chunk *os)
{
   LOG_FUNC_ENTRY();

   Chunk *cs = Chunk::NullChunkPtr;

   if (os != nullptr)
   {
      cs = os->GetNext();
   }

   while (  cs->IsNotNullChunk()
         && cs->level > os->level)
   {
      cs = cs->GetNext();
   }

   if (  cs->IsNullChunk()
      || chunk_is_not_token(cs, CT_SQUARE_CLOSE))
   {
      return;
   }
   LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, os->orig_line, os->orig_col);

   Chunk *tmp = cs->GetNextNcNnl();

   if (chunk_is_semicolon(tmp))
   {
      set_chunk_parent(tmp, CT_OC_MSG);
   }
   // expect a word first thing or [...]
   tmp = Chunk::NullChunkPtr;

   if (os != nullptr)
   {
      tmp = os->GetNextNcNnl();
   }

   if (  chunk_is_token(tmp, CT_SQUARE_OPEN)
      || chunk_is_token(tmp, CT_PAREN_OPEN)
      || chunk_is_token(tmp, CT_OC_AT))
   {
      Chunk *tt = tmp->GetNextNcNnl();

      if (  chunk_is_token(tmp, CT_OC_AT)
         && tt->IsNotNullChunk())
      {
         if (  chunk_is_token(tt, CT_PAREN_OPEN)
            || chunk_is_token(tt, CT_BRACE_OPEN)
            || chunk_is_token(tt, CT_SQUARE_OPEN))
         {
            tmp = tt;
         }
         else
         {
            LOG_FMT(LOCMSG, "%s(%d): tmp->orig_line is %zu, tmp->orig_col is %zu, expected identifier, not '%s' [%s]\n",
                    __func__, __LINE__, tmp->orig_line, tmp->orig_col,
                    tmp->Text(), get_token_name(tmp->type));
            return;
         }
      }
      tmp = chunk_skip_to_match(tmp);
   }
   else if (  chunk_is_not_token(tmp, CT_WORD)
           && chunk_is_not_token(tmp, CT_TYPE)
           && chunk_is_not_token(tmp, CT_THIS)
           && chunk_is_not_token(tmp, CT_STAR)
           && chunk_is_not_token(tmp, CT_STRING))
   {
      LOG_FMT(LOCMSG, "%s(%d): orig_line is %zu, orig_col is %zu, expected identifier, not '%s' [%s]\n",
              __func__, __LINE__, tmp->orig_line, tmp->orig_col,
              tmp->Text(), get_token_name(tmp->type));
      return;
   }
   else
   {
      if (tmp->IsStar()) // Issue #2722
      {
         set_chunk_type(tmp, CT_PTR_TYPE);
         tmp = tmp->GetNextNcNnl();
      }
      Chunk *tt = tmp->GetNextNcNnl();

      if (chunk_is_paren_open(tt))
      {
         LOG_FMT(LFCN, "%s(%d): (18) SET TO CT_FUNC_CALL: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, tmp->orig_line, tmp->orig_col, tmp->Text());
         set_chunk_type(tmp, CT_FUNC_CALL);
         tmp = set_paren_parent(tt, CT_FUNC_CALL)->GetPrevNcNnlNi();   // Issue #2279
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
   tmp = tmp->GetNextNcNnl();

   if (chunk_is_str(tmp, "<"))
   {
      Chunk *ao = tmp;
      Chunk *ac = ao->GetNextString(">", 1, ao->level);

      if (ac->IsNotNullChunk())
      {
         set_chunk_type(ao, CT_ANGLE_OPEN);
         set_chunk_parent(ao, CT_OC_PROTO_LIST);
         set_chunk_type(ac, CT_ANGLE_CLOSE);
         set_chunk_parent(ac, CT_OC_PROTO_LIST);

         for (tmp = ao->GetNext(); tmp != ac; tmp = tmp->GetNext())
         {
            tmp->level += 1;
            set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         }

         tmp = ac->GetNextNcNnl();
      }
      else
      {
         tmp = Chunk::NullChunkPtr;
      }
   }
   // handle 'object.property' and 'collection[index]'
   else
   {
      while (tmp->IsNotNullChunk())
      {
         if (chunk_is_token(tmp, CT_MEMBER))  // move past [object.prop1.prop2
         {
            Chunk *typ = tmp->GetNextNcNnl();

            if (  chunk_is_token(typ, CT_WORD)
               || chunk_is_token(typ, CT_TYPE))
            {
               tmp = typ->GetNextNcNnl();
            }
            else
            {
               break;
            }
         }
         else if (chunk_is_token(tmp, CT_SQUARE_OPEN))  // move past [collection[index]
         {
            Chunk *tcs = tmp->GetNextNcNnl();

            while (  tcs->IsNotNullChunk()
                  && tcs->level > tmp->level)
            {
               tcs = tcs->GetNextNcNnl();
            }

            if (chunk_is_token(tcs, CT_SQUARE_CLOSE))
            {
               tmp = tcs->GetNextNcNnl();
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
      tmp = chunk_skip_to_match(tmp)->GetNextNcNnl();
   }

   if (  chunk_is_token(tmp, CT_WORD)
      || chunk_is_token(tmp, CT_TYPE))
   {
      set_chunk_type(tmp, CT_OC_MSG_FUNC);
   }
   Chunk *prev = Chunk::NullChunkPtr;

   for (tmp = os->GetNext(); tmp != cs; tmp = tmp->GetNext())
   {
      chunk_flags_set(tmp, PCF_IN_OC_MSG);

      if (tmp->level == cs->level + 1)
      {
         if (chunk_is_token(tmp, CT_COLON))
         {
            set_chunk_type(tmp, CT_OC_COLON);

            if (  chunk_is_token(prev, CT_WORD)
               || chunk_is_token(prev, CT_TYPE))
            {
               // Might be a named param, check previous block
               Chunk *pp = prev->GetPrev();

               if (  pp->IsNotNullChunk()
                  && chunk_is_not_token(pp, CT_OC_COLON)
                  && chunk_is_not_token(pp, CT_ARITH)
                  && chunk_is_not_token(pp, CT_SHIFT)
                  && chunk_is_not_token(pp, CT_CARET))
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


static void handle_oc_available(Chunk *os)
{
   if (os != nullptr)
   {
      os = os->GetNext();
   }
   else
   {
      os = Chunk::NullChunkPtr;
   }

   while (os->IsNotNullChunk())
   {
      E_Token origType = os->type;
      set_chunk_type(os, CT_OC_AVAILABLE_VALUE);

      if (origType == CT_PAREN_CLOSE)
      {
         break;
      }
      os = os->GetNext();
   }
}


static void handle_oc_property_decl(Chunk *os)
{
   log_rule_B("mod_sort_oc_properties");

   if (options::mod_sort_oc_properties())
   {
      typedef std::vector<Chunk *> ChunkGroup;

      Chunk *next = Chunk::NullChunkPtr;

      if (os != nullptr)
      {
         next = os->GetNext();
      }
      Chunk                   *open_paren = nullptr;

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
         next       = next->GetNext();

         /*
          * Determine location of the property attributes
          * NOTE: Did not do this in the combine.cpp do_symbol_check as
          * I was not sure what the ramifications of adding a new type
          * for each of the below types would be. It did break some items
          * when I attempted to add them so this is my hack for now.
          */
         while (  next->IsNotNullChunk()
               && chunk_is_not_token(next, CT_PAREN_CLOSE))
         {
            if (chunk_is_token(next, CT_OC_PROPERTY_ATTR))
            {
               if (  chunk_is_str(next, "atomic")
                  || chunk_is_str(next, "nonatomic"))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  thread_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "readonly")
                       || chunk_is_str(next, "readwrite"))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  readwrite_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "assign")
                       || chunk_is_str(next, "retain")
                       || chunk_is_str(next, "copy")
                       || chunk_is_str(next, "strong")
                       || chunk_is_str(next, "weak")
                       || chunk_is_str(next, "unsafe_unretained"))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  ref_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "getter"))
               {
                  ChunkGroup chunkGroup;

                  do
                  {
                     chunkGroup.push_back(next);
                     next = next->GetNext();
                  } while (  next->IsNotNullChunk()
                          && chunk_is_not_token(next, CT_COMMA)
                          && chunk_is_not_token(next, CT_PAREN_CLOSE));

                  next = next->GetPrev();

                  // coverity CID 160946
                  if (next->IsNullChunk())
                  {
                     break;
                  }
                  getter_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "setter"))
               {
                  ChunkGroup chunkGroup;

                  do
                  {
                     chunkGroup.push_back(next);
                     next = next->GetNext();
                  } while (  next->IsNotNullChunk()
                          && chunk_is_not_token(next, CT_COMMA)
                          && chunk_is_not_token(next, CT_PAREN_CLOSE));

                  if (next->IsNotNullChunk())
                  {
                     next = next->GetPrev();
                  }

                  if (next->IsNullChunk())
                  {
                     break;
                  }
                  setter_chunks.push_back(chunkGroup);
               }
               else if (  chunk_is_str(next, "nullable")
                       || chunk_is_str(next, "nonnull")
                       || chunk_is_str(next, "null_resettable")
                       || chunk_is_str(next, "null_unspecified"))
               {
                  ChunkGroup chunkGroup;
                  chunkGroup.push_back(next);
                  nullability_chunks.push_back(chunkGroup);
               }
               else if (chunk_is_str(next, "class"))
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
               if (chunk_is_str(next, "class"))
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
            next = next->GetNext();
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

         Chunk *curr_chunk = open_paren;

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
                     curr_chunk = curr_chunk->GetNext();
                  }
               }

               // add the parenthesis
               Chunk endchunk;
               set_chunk_type(&endchunk, CT_COMMA);
               set_chunk_parent(&endchunk, get_chunk_parent_type(curr_chunk));
               endchunk.str         = ",";
               endchunk.level       = curr_chunk->level;
               endchunk.pp_level    = curr_chunk->pp_level;
               endchunk.brace_level = curr_chunk->brace_level;
               endchunk.orig_line   = curr_chunk->orig_line;
               endchunk.orig_col    = curr_chunk->orig_col;
               endchunk.column      = curr_chunk->orig_col_end + 1;
               endchunk.flags       = curr_chunk->flags & PCF_COPY_FLAGS;
               chunk_add_after(&endchunk, curr_chunk);
               curr_chunk = curr_chunk->GetNext();
            }
         }

         // Remove the extra comma's that we did not move
         while (  curr_chunk != nullptr
               && curr_chunk->IsNotNullChunk()
               && chunk_is_not_token(curr_chunk, CT_PAREN_CLOSE))
         {
            Chunk *rm_chunk = curr_chunk;
            curr_chunk = curr_chunk->GetNext();
            chunk_del(rm_chunk);
         }
      }
   }
   Chunk *tmp = Chunk::NullChunkPtr;

   if (os != nullptr)
   {
      tmp = os->GetNextNcNnl();
   }

   if (chunk_is_paren_open(tmp))
   {
      tmp = chunk_skip_to_match(tmp)->GetNextNcNnl();
   }
   fix_variable_definition(tmp);
} // handle_oc_property_decl


static void handle_cs_square_stmt(Chunk *os)
{
   LOG_FUNC_ENTRY();

   if (os == nullptr)
   {
      os = Chunk::NullChunkPtr;
   }
   Chunk *cs = os->GetNext();

   while (  cs->IsNotNullChunk()
         && cs->level > os->level)
   {
      cs = cs->GetNext();
   }

   if (  cs->IsNullChunk()
      || chunk_is_not_token(cs, CT_SQUARE_CLOSE))
   {
      return;
   }
   set_chunk_parent(os, CT_CS_SQ_STMT);
   set_chunk_parent(cs, CT_CS_SQ_STMT);

   Chunk *tmp;

   for (tmp = os->GetNext(); tmp != cs; tmp = tmp->GetNext())
   {
      set_chunk_parent(tmp, CT_CS_SQ_STMT);

      if (chunk_is_token(tmp, CT_COLON))
      {
         set_chunk_type(tmp, CT_CS_SQ_COLON);
      }
   }

   tmp = cs->GetNextNcNnl();

   if (tmp->IsNotNullChunk())
   {
      chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
   }
} // handle_cs_square_stmt


static void handle_cs_property(Chunk *bro)
{
   LOG_FUNC_ENTRY();

   set_paren_parent(bro, CT_CS_PROPERTY);

   bool  did_prop = false;
   Chunk *pc      = bro;

   while ((pc = pc->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
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
            && (  chunk_is_token(pc, CT_WORD)
               || chunk_is_token(pc, CT_THIS)))
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


static void handle_cs_array_type(Chunk *pc)
{
   if (  pc == nullptr
      || pc->IsNullChunk())
   {
      return;
   }
   Chunk *prev = pc->GetPrev();

   for ( ;
         chunk_is_token(prev, CT_COMMA);
         prev = prev->GetPrev())
   {
      // empty
   }

   if (chunk_is_token(prev, CT_SQUARE_OPEN))
   {
      while (pc != prev)
      {
         set_chunk_parent(pc, CT_TYPE);
         pc = pc->GetPrev();
      }
      set_chunk_parent(prev, CT_TYPE);
   }
}


static void handle_wrap(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *opp = Chunk::NullChunkPtr;

   if (pc != nullptr)
   {
      opp = pc->GetNext();
   }
   Chunk *name = opp->GetNext();
   Chunk *clp  = name->GetNext();

   log_rule_B("sp_func_call_paren");
   log_rule_B("sp_cpp_cast_paren");
   iarf_e pav = chunk_is_token(pc, CT_FUNC_WRAP) ?
                options::sp_func_call_paren() :
                options::sp_cpp_cast_paren();

   log_rule_B("sp_inside_fparen");
   log_rule_B("sp_inside_paren_cast");
   iarf_e av = chunk_is_token(pc, CT_FUNC_WRAP) ?
               options::sp_inside_fparen() :
               options::sp_inside_paren_cast();

   if (  chunk_is_token(clp, CT_PAREN_CLOSE)
      && chunk_is_token(opp, CT_PAREN_OPEN)
      && (  chunk_is_token(name, CT_WORD)
         || chunk_is_token(name, CT_TYPE)))
   {
      const char *psp = (pav & IARF_ADD) ? " " : "";
      const char *fsp = (av & IARF_ADD) ? " " : "";

      pc->str.append(psp);
      pc->str.append("(");
      pc->str.append(fsp);
      pc->str.append(name->str);
      pc->str.append(fsp);
      pc->str.append(")");

      set_chunk_type(pc, chunk_is_token(pc, CT_FUNC_WRAP) ? CT_FUNCTION : CT_TYPE);

      pc->orig_col_end = pc->orig_col + pc->Len();

      chunk_del(opp);
      chunk_del(name);
      chunk_del(clp);
   }
} // handle_wrap


static void handle_proto_wrap(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *opp  = pc->GetNextNcNnl();
   Chunk *name = opp->GetNextNcNnl();
   Chunk *tmp  = name->GetNextNcNnl()->GetNextNcNnl();
   Chunk *clp  = chunk_skip_to_match(opp);
   Chunk *cma  = clp->GetNextNcNnl();

   if (  opp->IsNullChunk()
      || name->IsNullChunk()
      || tmp->IsNullChunk()
      || clp == nullptr
      || cma->IsNullChunk()
      || (  chunk_is_not_token(name, CT_WORD)
         && chunk_is_not_token(name, CT_TYPE))
      || chunk_is_not_token(opp, CT_PAREN_OPEN))
   {
      return;
   }

   if (chunk_is_token(cma, CT_SEMICOLON))
   {
      set_chunk_type(pc, CT_FUNC_PROTO);
   }
   else if (chunk_is_token(cma, CT_BRACE_OPEN))
   {
      LOG_FMT(LFCN, "%s(%d): (19) SET TO CT_FUNC_DEF: orig_line is %zu, orig_col is %zu, Text() '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
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

   while ((tmp = tmp->GetPrevNcNnlNi())->IsNotNullChunk()) // Issue #2279
   {
      if (  !chunk_is_type(tmp)
         && chunk_is_not_token(tmp, CT_OPERATOR)
         && chunk_is_not_token(tmp, CT_WORD)
         && chunk_is_not_token(tmp, CT_ADDR))
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
static void handle_java_assert(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   bool  did_colon = false;
   Chunk *tmp      = pc;

   if (tmp == nullptr)
   {
      tmp = Chunk::NullChunkPtr;
   }

   while ((tmp = tmp->GetNext())->IsNotNullChunk())
   {
      if (tmp->level == pc->level)
      {
         if (  !did_colon
            && chunk_is_token(tmp, CT_COLON))
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
