/**
 * @file combine.cpp
 * Labels the chunks as needed.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "prototypes.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"
#include <cassert>
#include <stack>

static void fix_fcn_def_params(chunk_t *pc);
static void fix_typedef(chunk_t *pc);
static void fix_enum_struct_union(chunk_t *pc);
static void fix_casts(chunk_t *pc);
static void fix_type_cast(chunk_t *pc);
static chunk_t *fix_var_def(chunk_t *pc);
static void mark_function(chunk_t *pc);
static void mark_function_return_type(chunk_t *fname, chunk_t *pc, c_token_t parent_type);
static bool mark_function_type(chunk_t *pc);
static void mark_struct_union_body(chunk_t *start);
static chunk_t *mark_variable_definition(chunk_t *start);

static void mark_define_expressions(void);
static void process_returns(void);
static chunk_t *process_return(chunk_t *pc);
static void mark_class_ctor(chunk_t *pclass);
static void mark_namespace(chunk_t *pns);
static void mark_cpp_constructor(chunk_t *pc);
static void mark_lvalue(chunk_t *pc);
static void mark_template_func(chunk_t *pc, chunk_t *pc_next);
static void mark_exec_sql(chunk_t *pc);
static void handle_oc_class(chunk_t *pc);
static void handle_oc_block_literal(chunk_t *pc);
static void handle_oc_block_type(chunk_t *pc);
static void handle_oc_message_decl(chunk_t *pc);
static void handle_oc_message_send(chunk_t *pc);
static void handle_cs_square_stmt(chunk_t *pc);
static void handle_cs_property(chunk_t *pc);
static void handle_cs_array_type(chunk_t *pc);
static void handle_cpp_template(chunk_t *pc);
static void handle_cpp_lambda(chunk_t *pc);
static void handle_d_template(chunk_t *pc);
static void handle_wrap(chunk_t *pc);
static void handle_proto_wrap(chunk_t *pc);
static bool is_oc_block(chunk_t *pc);
static void handle_java_assert(chunk_t *pc);
static chunk_t *get_d_template_types(ChunkStack& cs, chunk_t *open_paren);
static bool chunkstack_match(ChunkStack& cs, chunk_t *pc);

void make_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   if (pc != NULL)
   {
      if (pc->type == CT_WORD)
      {
         set_chunk_type(pc, CT_TYPE);
      }
      else if (chunk_is_star(pc) || chunk_is_msref(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_addr(pc))
      {
         set_chunk_type(pc, CT_BYREF);
      }
   }
}


void flag_series(chunk_t *start, chunk_t *end, UINT64 set_flags, UINT64 clr_flags, chunk_nav_t nav)
{
   LOG_FUNC_ENTRY();
   while (start && (start != end))
   {
      start->flags = (start->flags & ~clr_flags) | set_flags;

      start = chunk_get_next(start, nav);
   }
   if (end)
   {
      end->flags = (end->flags & ~clr_flags) | set_flags;
   }
}


/**
 * Flags everything from the open paren to the close paren.
 *
 * @param po   Pointer to the open parenthesis
 * @return     The token after the close paren
 */
static chunk_t *flag_parens(chunk_t *po, UINT64 flags,
                            c_token_t opentype, c_token_t parenttype,
                            bool parent_all)
{
   LOG_FUNC_ENTRY();
   chunk_t *paren_close;
   chunk_t *pc;

   paren_close = chunk_skip_to_match(po, CNAV_PREPROC);
   if (paren_close == NULL)
   {
      LOG_FMT(LERR, "flag_parens: no match for [%s] at [%d:%d]",
              po->str.c_str(), po->orig_line, po->orig_col);
      log_func_stack_inline(LERR);
      return(NULL);
   }

   LOG_FMT(LFLPAREN, "flag_parens: %d:%d [%s] and %d:%d [%s] type=%s ptype=%s",
           po->orig_line, po->orig_col, po->text(),
           paren_close->orig_line, paren_close->orig_col, paren_close->text(),
           get_token_name(opentype), get_token_name(parenttype));
   log_func_stack_inline(LSETTYP);

   if (po != paren_close)
   {
      if ((flags != 0) ||
          (parent_all && (parenttype != CT_NONE)))
      {
         for (pc = chunk_get_next(po, CNAV_PREPROC);
              pc != paren_close;
              pc = chunk_get_next(pc, CNAV_PREPROC))
         {
            pc->flags |= flags;
            if (parent_all)
            {
               set_chunk_parent(pc, parenttype);
            }
         }
      }

      if (opentype != CT_NONE)
      {
         set_chunk_type(po, opentype);
         set_chunk_type(paren_close, (c_token_t)(opentype + 1));
      }

      if (parenttype != CT_NONE)
      {
         set_chunk_parent(po, parenttype);
         set_chunk_parent(paren_close, parenttype);
      }
   }
   return(chunk_get_next_ncnl(paren_close, CNAV_PREPROC));
}


/**
 * Sets the parent of the open paren/brace/square/angle and the closing.
 * Note - it is assumed that pc really does point to an open item and the
 * close must be open + 1.
 *
 * @param start   The open paren
 * @param parent  The type to assign as the parent
 * @reutrn        The chunk after the close paren
 */
chunk_t *set_paren_parent(chunk_t *start, c_token_t parent)
{
   LOG_FUNC_ENTRY();
   chunk_t *end;

   end = chunk_skip_to_match(start, CNAV_PREPROC);
   if (end != NULL)
   {
      LOG_FMT(LFLPAREN, "set_paren_parent: %d:%d [%s] and %d:%d [%s] type=%s ptype=%s",
              start->orig_line, start->orig_col, start->text(),
              end->orig_line, end->orig_col, end->text(),
              get_token_name(start->type), get_token_name(parent));
      log_func_stack_inline(LFLPAREN);
      set_chunk_parent(start, parent);
      set_chunk_parent(end, parent);
   }
   return(chunk_get_next_ncnl(end, CNAV_PREPROC));
}


/* Scan backwards to see if we might be on a type declaration */
static bool chunk_ends_type(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc       = start;
   bool    ret       = false;
   int     cnt       = 0;
   bool    last_lval = false;

   for (/* nada */; pc != NULL; pc = chunk_get_prev_ncnl(pc))
   {
      LOG_FMT(LFTYPE, "%s: [%s] %s flags %" PRIx64 " on line %d, col %d\n",
              __func__, get_token_name(pc->type), pc->str.c_str(),
              pc->flags, pc->orig_line, pc->orig_col);

      if ((pc->type == CT_WORD) ||
          (pc->type == CT_TYPE) ||
          (pc->type == CT_PTR_TYPE) ||
          (pc->type == CT_STRUCT) ||
          (pc->type == CT_DC_MEMBER) ||
          (pc->type == CT_QUALIFIER))
      {
         cnt++;
         last_lval = (pc->flags & PCF_LVALUE) != 0;
         continue;
      }

      if (chunk_is_semicolon(pc) ||
          (pc->type == CT_TYPEDEF) ||
          (pc->type == CT_BRACE_OPEN) ||
          (pc->type == CT_BRACE_CLOSE) ||
          chunk_is_forin(pc) ||
          ((pc->type == CT_SPAREN_OPEN) && last_lval))
      {
         ret = cnt > 0;
      }
      break;
   }

   if (pc == NULL)
   {
      /* first token */
      ret = true;
   }

   LOG_FMT(LFTYPE, "%s verdict: %s\n", __func__, ret ? "yes" : "no");

   return(ret);
}


/* skip to the final word/type in a :: chain
 * pc is either a word or a ::
 */
static chunk_t *skip_dc_member(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   if (!start)
   {
      return NULL;
   }

   chunk_t *pc   = start;
   chunk_t *next = (pc->type == CT_DC_MEMBER) ? pc : chunk_get_next_ncnl(pc);
   while (next && (next->type == CT_DC_MEMBER))
   {
      pc   = chunk_get_next_ncnl(next);
      next = chunk_get_next_ncnl(pc);
   }
   return pc;
}


/**
 * This is called on every chunk.
 * First on all non-preprocessor chunks and then on each preprocessor chunk.
 * It does all the detection and classifying.
 */
void do_symbol_check(chunk_t *prev, chunk_t *pc, chunk_t *next)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   // LOG_FMT(LSYS, " %3d > ['%s' %s] ['%s' %s] ['%s' %s]\n",
   //         pc->orig_line,
   //         prev->str.c_str(), get_token_name(prev->type),
   //         pc->str.c_str(), get_token_name(pc->type),
   //         next->str.c_str(), get_token_name(next->type));

   if ((pc->type == CT_OC_AT) && next)
   {
      if ((next->type == CT_PAREN_OPEN) ||
          (next->type == CT_BRACE_OPEN) ||
          (next->type == CT_SQUARE_OPEN))
      {
         flag_parens(next, PCF_OC_BOXED, next->type, CT_OC_AT, false);
      }
      else
      {
         set_chunk_parent(next, CT_OC_AT);
      }
   }

   /* D stuff */
   if ((cpd.lang_flags & LANG_D) &&
       (pc->type == CT_QUALIFIER) &&
       chunk_is_str(pc, "const", 5) &&
       (next->type == CT_PAREN_OPEN))
   {
      set_chunk_type(pc, CT_D_CAST);
      set_paren_parent(next, pc->type);
   }

   if ((next->type == CT_PAREN_OPEN) &&
       ((pc->type == CT_D_CAST) ||
        (pc->type == CT_DELEGATE) ||
        (pc->type == CT_ALIGN)))
   {
      /* mark the parenthesis parent */
      tmp = set_paren_parent(next, pc->type);

      /* For a D cast - convert the next item */
      if ((pc->type == CT_D_CAST) && (tmp != NULL))
      {
         if (tmp->type == CT_STAR)
         {
            set_chunk_type(tmp, CT_DEREF);
         }
         else if (tmp->type == CT_AMP)
         {
            set_chunk_type(tmp, CT_ADDR);
         }
         else if (tmp->type == CT_MINUS)
         {
            set_chunk_type(tmp, CT_NEG);
         }
         else if (tmp->type == CT_PLUS)
         {
            set_chunk_type(tmp, CT_POS);
         }
      }

      /* For a delegate, mark previous words as types and the item after the
       * close paren as a variable def
       */
      if (pc->type == CT_DELEGATE)
      {
         if (tmp != NULL)
         {
            set_chunk_parent(tmp, CT_DELEGATE);
            if (tmp->level == tmp->brace_level)
            {
               tmp->flags |= PCF_VAR_1ST_DEF;
            }
         }

         for (tmp = chunk_get_prev_ncnl(pc); tmp != NULL; tmp = chunk_get_prev_ncnl(tmp))
         {
            if (chunk_is_semicolon(tmp) ||
                (tmp->type == CT_BRACE_OPEN) ||
                (tmp->type == CT_VBRACE_OPEN))
            {
               break;
            }
            make_type(tmp);
         }
      }

      if ((pc->type == CT_ALIGN) && (tmp != NULL))
      {
         if (tmp->type == CT_BRACE_OPEN)
         {
            set_paren_parent(tmp, pc->type);
         }
         else if (tmp->type == CT_COLON)
         {
            set_chunk_parent(tmp, pc->type);
         }
      }
   } /* paren open + cast/align/delegate */

   if (pc->type == CT_INVARIANT)
   {
      if (next->type == CT_PAREN_OPEN)
      {
         set_chunk_parent(next, pc->type);
         tmp = chunk_get_next(next);
         while (tmp != NULL)
         {
            if (tmp->type == CT_PAREN_CLOSE)
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

   if ((prev->type == CT_BRACE_OPEN) &&
       (prev->parent_type != CT_CS_PROPERTY) &&
       ((pc->type == CT_GETSET) || (pc->type == CT_GETSET_EMPTY)))
   {
      flag_parens(prev, 0, CT_NONE, CT_GETSET, false);
   }


   /* Objective C stuff */
   if (cpd.lang_flags & LANG_OC)
   {
      /* Check for message declarations */
      if (pc->flags & PCF_STMT_START)
      {
         if ((chunk_is_str(pc, "-", 1) || chunk_is_str(pc, "+", 1)) &&
             chunk_is_str(next, "(", 1))
         {
            handle_oc_message_decl(pc);
         }
      }
      if (pc->flags & PCF_EXPR_START)
      {
         if (pc->type == CT_SQUARE_OPEN)
         {
            handle_oc_message_send(pc);
         }
         if (pc->type == CT_CARET)
         {
            handle_oc_block_literal(pc);
         }
      }
   }

   /* C# stuff */
   if (cpd.lang_flags & LANG_CS)
   {
      /* '[assembly: xxx]' stuff */
      if ((pc->flags & PCF_EXPR_START) &&
          (pc->type == CT_SQUARE_OPEN))
      {
         handle_cs_square_stmt(pc);
      }

      if ((next != NULL) && (next->type == CT_BRACE_OPEN) &&
          (next->parent_type == CT_NONE) &&
          ((pc->type == CT_SQUARE_CLOSE) ||
           (pc->type == CT_ANGLE_CLOSE) ||
           (pc->type == CT_WORD)))
      {
         handle_cs_property(next);
      }

      if ((pc->type == CT_SQUARE_CLOSE) &&
          (next != NULL) && (next->type == CT_WORD))
      {
         handle_cs_array_type(pc);
      }

      if ((((pc->type == CT_LAMBDA) || (pc->type == CT_DELEGATE))) && (next->type == CT_BRACE_OPEN))
      {
         set_paren_parent(next, pc->type);
      }
   }

   if (pc->type == CT_NEW)
   {
      chunk_t *ts = NULL;
      tmp = next;
      if (tmp->type == CT_TSQUARE)
      {
         ts = tmp;
         tmp = chunk_get_next_ncnl(tmp);
      }
      if (tmp && (tmp->type == CT_BRACE_OPEN))
      {
         set_paren_parent(tmp, pc->type);
         if (ts)
         {
            ts->parent_type = pc->type;
         }
      }
   }

   /* C++11 Lambda stuff */
   if (prev && (cpd.lang_flags & LANG_CPP) &&
       ((pc->type == CT_SQUARE_OPEN) || (pc->type == CT_TSQUARE)) &&
       !CharTable::IsKw1(prev->str[0]))
   {
      handle_cpp_lambda(pc);
   }

   /* FIXME: which language does this apply to? */
   if ((pc->type == CT_ASSIGN) && (next->type == CT_SQUARE_OPEN))
   {
      set_paren_parent(next, CT_ASSIGN);

      /* Mark one-liner assignment */
      tmp = next;
      while ((tmp = chunk_get_next_nc(tmp)) != NULL)
      {
         if (chunk_is_newline(tmp))
         {
            break;
         }
         if ((tmp->type == CT_SQUARE_CLOSE) && (next->level == tmp->level))
         {
            tmp->flags  |= PCF_ONE_LINER;
            next->flags |= PCF_ONE_LINER;
            break;
         }
      }
   }

   if (pc->type == CT_ASSERT)
   {
      handle_java_assert(pc);
   }
   if (pc->type == CT_ANNOTATION)
   {
      tmp = chunk_get_next_ncnl(pc);
      if (chunk_is_paren_open(tmp))
      {
         set_paren_parent(tmp, CT_ANNOTATION);
      }
   }

   /* A [] in C# and D only follows a type */
   if ((pc->type == CT_TSQUARE) &&
       ((cpd.lang_flags & (LANG_D | LANG_CS | LANG_VALA)) != 0))
   {
      if ((prev != NULL) && (prev->type == CT_WORD))
      {
         set_chunk_type(prev, CT_TYPE);
      }
      if ((next != NULL) && (next->type == CT_WORD))
      {
         next->flags |= PCF_VAR_1ST_DEF;
      }
   }

   if ((pc->type == CT_SQL_EXEC) ||
       (pc->type == CT_SQL_BEGIN) ||
       (pc->type == CT_SQL_END))
   {
      mark_exec_sql(pc);
   }

   if (pc->type == CT_PROTO_WRAP)
   {
      handle_proto_wrap(pc);
   }

   /* Handle the typedef */
   if (pc->type == CT_TYPEDEF)
   {
      fix_typedef(pc);
   }
   if ((pc->type == CT_ENUM) ||
       (pc->type == CT_STRUCT) ||
       (pc->type == CT_UNION))
   {
      if (prev->type != CT_TYPEDEF)
      {
         fix_enum_struct_union(pc);
      }
   }

   if (pc->type == CT_EXTERN)
   {
      if (chunk_is_paren_open(next))
      {
         tmp = flag_parens(next, 0, CT_NONE, CT_EXTERN, true);
         if (tmp && (tmp->type == CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, CT_EXTERN);
         }
      }
      else
      {
         /* next likely is a string (see tokenize_cleanup.cpp) */
         set_chunk_parent(next, CT_EXTERN);
         tmp = chunk_get_next_ncnl(next);
         if (tmp && (tmp->type == CT_BRACE_OPEN))
         {
            set_paren_parent(tmp, CT_EXTERN);
         }
      }
   }

   if (pc->type == CT_TEMPLATE)
   {
      if (cpd.lang_flags & LANG_D)
      {
         handle_d_template(pc);
      }
      else
      {
         handle_cpp_template(pc);
      }
   }

   if ((pc->type == CT_WORD) &&
       (next->type == CT_ANGLE_OPEN) &&
       (next->parent_type == CT_TEMPLATE))
   {
      mark_template_func(pc, next);
   }

   if ((pc->type == CT_SQUARE_CLOSE) &&
       (next->type == CT_PAREN_OPEN))
   {
      flag_parens(next, 0, CT_FPAREN_OPEN, CT_NONE, false);
   }

   if (pc->type == CT_TYPE_CAST)
   {
      fix_type_cast(pc);
   }

   if ((pc->parent_type == CT_ASSIGN) &&
       ((pc->type == CT_BRACE_OPEN) ||
        (pc->type == CT_SQUARE_OPEN)))
   {
      /* Mark everything in here as in assign */
      flag_parens(pc, PCF_IN_ARRAY_ASSIGN, pc->type, CT_NONE, false);
   }

   if (pc->type == CT_D_TEMPLATE)
   {
      set_paren_parent(next, pc->type);
   }

   /**
    * A word before an open paren is a function call or definition.
    * CT_WORD => CT_FUNC_CALL or CT_FUNC_DEF
    */
   if (next->type == CT_PAREN_OPEN)
   {
      tmp = chunk_get_next_ncnl(next);
      if ((cpd.lang_flags & LANG_OC) && chunk_is_token(tmp, CT_CARET))
      {
         handle_oc_block_type(tmp);

         // This is the case where a block literal is passed as the first argument of a C-style method invocation.
         if ((tmp->type == CT_OC_BLOCK_CARET) && (pc->type == CT_WORD))
         {
            set_chunk_type(pc, CT_FUNC_CALL);
         }
      }
      else if ((pc->type == CT_WORD) || (pc->type == CT_OPERATOR_VAL))
      {
         set_chunk_type(pc, CT_FUNCTION);
      }
      else if (pc->type == CT_TYPE)
      {
         /**
          * If we are on a type, then we are either on a C++ style cast, a
          * function or we are on a function type.
          * The only way to tell for sure is to find the close paren and see
          * if it is followed by an open paren.
          * "int(5.6)"
          * "int()"
          * "int(foo)(void)"
          *
          * FIXME: this check can be done better...
          */
         tmp = chunk_get_next_type(next, CT_PAREN_CLOSE, next->level);
         tmp = chunk_get_next(tmp);
         if ((tmp != NULL) && (tmp->type == CT_PAREN_OPEN))
         {
            /* we have "TYPE(...)(" */
            set_chunk_type(pc, CT_FUNCTION);
         }
         else
         {
            if ((pc->parent_type == CT_NONE) &&
                ((pc->flags & PCF_IN_TYPEDEF) == 0))
            {
               tmp = chunk_get_next_ncnl(next);
               if ((tmp != NULL) && (tmp->type == CT_PAREN_CLOSE))
               {
                  /* we have TYPE() */
                  set_chunk_type(pc, CT_FUNCTION);
               }
               else
               {
                  /* we have TYPE(...) */
                  set_chunk_type(pc, CT_CPP_CAST);
                  set_paren_parent(next, CT_CPP_CAST);
               }
            }
         }
      }
      else if (pc->type == CT_ATTRIBUTE)
      {
         flag_parens(next, 0, CT_FPAREN_OPEN, CT_ATTRIBUTE, false);
      }
   }
   if ((cpd.lang_flags & LANG_PAWN) != 0)
   {
      if ((pc->type == CT_FUNCTION) && (pc->brace_level > 0))
      {
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      if ((pc->type == CT_STATE) &&
          (next != NULL) &&
          (next->type == CT_PAREN_OPEN))
      {
         set_paren_parent(next, pc->type);
      }
   }
   else
   {
      if ((pc->type == CT_FUNCTION) &&
          ((pc->parent_type == CT_OC_BLOCK_EXPR) || !is_oc_block(pc)))
      {
         mark_function(pc);
      }
   }

   /* Detect C99 member stuff */
   if ((pc->type == CT_MEMBER) &&
       ((prev->type == CT_COMMA) ||
        (prev->type == CT_BRACE_OPEN)))
   {
      set_chunk_type(pc, CT_C99_MEMBER);
      set_chunk_parent(next, CT_C99_MEMBER);
   }

   /* Mark function parens and braces */
   if ((pc->type == CT_FUNC_DEF) ||
       (pc->type == CT_FUNC_CALL) ||
       (pc->type == CT_FUNC_CALL_USER) ||
       (pc->type == CT_FUNC_PROTO))
   {
      tmp = next;
      if (tmp->type == CT_SQUARE_OPEN)
      {
         tmp = set_paren_parent(tmp, pc->type);
      }
      else if ((tmp->type == CT_TSQUARE) ||
               (tmp->parent_type == CT_OPERATOR))
      {
         tmp = chunk_get_next_ncnl(tmp);
      }

      if (chunk_is_paren_open(tmp))
      {
         tmp = flag_parens(tmp, 0, CT_FPAREN_OPEN, pc->type, false);
         if (tmp != NULL)
         {
            if (tmp->type == CT_BRACE_OPEN)
            {
               if ((tmp->parent_type != CT_DOUBLE_BRACE) &&
                   ((pc->flags & PCF_IN_CONST_ARGS) == 0))
               {
                  set_paren_parent(tmp, pc->type);
               }
            }
            else if (chunk_is_semicolon(tmp) && (pc->type == CT_FUNC_PROTO))
            {
               set_chunk_parent(tmp, pc->type);
            }
         }
      }
   }

   /* Mark the parameters in catch() */
   if ((pc->type == CT_CATCH) && (next->type == CT_SPAREN_OPEN))
   {
      fix_fcn_def_params(next);
   }

   if ((pc->type == CT_THROW) && (prev->type == CT_FPAREN_CLOSE))
   {
      set_chunk_parent(pc, prev->parent_type);
      if (next->type == CT_PAREN_OPEN)
      {
         set_paren_parent(next, CT_THROW);
      }
   }

   /* Mark the braces in: "for_each_entry(xxx) { }" */
   if ((pc->type == CT_BRACE_OPEN) &&
       (pc->parent_type != CT_DOUBLE_BRACE) &&
       (prev->type == CT_FPAREN_CLOSE) &&
       ((prev->parent_type == CT_FUNC_CALL) ||
        (prev->parent_type == CT_FUNC_CALL_USER)) &&
       ((pc->flags & PCF_IN_CONST_ARGS) == 0))
   {
      set_paren_parent(pc, CT_FUNC_CALL);
   }

   /* Check for a close paren followed by an open paren, which means that
    * we are on a function type declaration (C/C++ only?).
    * Note that typedefs are already taken care of.
    */
   if ((next != NULL) &&
       ((pc->flags & (PCF_IN_TYPEDEF | PCF_IN_TEMPLATE)) == 0) &&
       (pc->parent_type != CT_CPP_CAST) &&
       (pc->parent_type != CT_C_CAST) &&
       ((pc->flags & PCF_IN_PREPROC) == 0) &&
       (!is_oc_block(pc)) &&
       (pc->parent_type != CT_OC_MSG_DECL) &&
       (pc->parent_type != CT_OC_MSG_SPEC) &&
       chunk_is_str(pc, ")", 1) &&
       chunk_is_str(next, "(", 1))
   {
      if ((cpd.lang_flags & LANG_D) != 0)
      {
         flag_parens(next, 0, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
      }
      else
      {
         mark_function_type(pc);
      }
   }

   if (((pc->type == CT_CLASS) ||
        (pc->type == CT_STRUCT)) &&
       (pc->level == pc->brace_level))
   {
      if ((pc->type != CT_STRUCT) || ((cpd.lang_flags & LANG_C) == 0))
      {
         mark_class_ctor(pc);
      }
   }

   if (pc->type == CT_OC_CLASS)
   {
      handle_oc_class(pc);
   }

   if (pc->type == CT_NAMESPACE)
   {
      mark_namespace(pc);
   }

   /*TODO: Check for stuff that can only occur at the start of an statement */

   if ((cpd.lang_flags & LANG_D) == 0)
   {
      /**
       * Check a paren pair to see if it is a cast.
       * Note that SPAREN and FPAREN have already been marked.
       */
      if ((pc->type == CT_PAREN_OPEN) &&
          ((pc->parent_type == CT_NONE) ||
           (pc->parent_type == CT_OC_MSG) ||
           (pc->parent_type == CT_OC_BLOCK_EXPR)) &&
          ((next->type == CT_WORD) ||
           (next->type == CT_TYPE) ||
           (next->type == CT_STRUCT) ||
           (next->type == CT_QUALIFIER) ||
           (next->type == CT_MEMBER) ||
           (next->type == CT_DC_MEMBER) ||
           (next->type == CT_ENUM) ||
           (next->type == CT_UNION)) &&
          (prev->type != CT_SIZEOF) &&
          (prev->parent_type != CT_OPERATOR) &&
          ((pc->flags & PCF_IN_TYPEDEF) == 0))
      {
         fix_casts(pc);
      }
   }


   /* Check for stuff that can only occur at the start of an expression */
   if ((pc->flags & PCF_EXPR_START) != 0)
   {
      /* Change STAR, MINUS, and PLUS in the easy cases */
      if (pc->type == CT_STAR)
      {
         set_chunk_type(pc, (prev->type == CT_ANGLE_CLOSE) ? CT_PTR_TYPE : CT_DEREF);
      }
      if ((cpd.lang_flags & LANG_CPP) && (pc->type == CT_CARET) && (prev->type == CT_ANGLE_CLOSE))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      if (pc->type == CT_MINUS)
      {
         set_chunk_type(pc, CT_NEG);
      }
      if (pc->type == CT_PLUS)
      {
         set_chunk_type(pc, CT_POS);
      }
      if (pc->type == CT_INCDEC_AFTER)
      {
         set_chunk_type(pc, CT_INCDEC_BEFORE);
         //fprintf(stderr, "%s: %d> changed INCDEC_AFTER to INCDEC_BEFORE\n", __func__, pc->orig_line);
      }
      if (pc->type == CT_AMP)
      {
         //fprintf(stderr, "Changed AMP to ADDR on line %d\n", pc->orig_line);
         set_chunk_type(pc, CT_ADDR);
      }
      if (pc->type == CT_CARET)
      {
         if (cpd.lang_flags & LANG_OC)
         {
            /* This is likely the start of a block literal */
            handle_oc_block_literal(pc);
         }
      }
   }

   /* Detect a variable definition that starts with struct/enum/union/class */
   if (((pc->flags & PCF_IN_TYPEDEF) == 0) &&
       (prev->parent_type != CT_CPP_CAST) &&
       ((prev->flags & PCF_IN_FCN_DEF) == 0) &&
       ((pc->type == CT_STRUCT) ||
        (pc->type == CT_UNION) ||
        (pc->type == CT_CLASS) ||
        (pc->type == CT_ENUM)))
   {
      tmp = skip_dc_member(next);
      if (tmp && ((tmp->type == CT_TYPE) || (tmp->type == CT_WORD)))
      {
         set_chunk_parent(tmp, pc->type);
         set_chunk_type(tmp, CT_TYPE);

         tmp = chunk_get_next_ncnl(tmp);
      }
      if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
      {
         tmp = chunk_skip_to_match(tmp);
         tmp = chunk_get_next_ncnl(tmp);
      }
      if ((tmp != NULL) && (chunk_is_ptr_operator(tmp) || (tmp->type == CT_WORD)))
      {
         mark_variable_definition(tmp);
      }
   }

   if (pc->type == CT_OC_PROPERTY)
   {
      tmp = chunk_get_next_ncnl(pc);
      if (chunk_is_paren_open(tmp))
      {
         tmp = chunk_get_next_ncnl(chunk_skip_to_match(tmp));
      }
      fix_var_def(tmp);
   }

   /**
    * Change the paren pair after a function/macrofunc.
    * CT_PAREN_OPEN => CT_FPAREN_OPEN
    */
   if (pc->type == CT_MACRO_FUNC)
   {
      flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_MACRO_FUNC, false);
   }

   if ((pc->type == CT_MACRO_OPEN) ||
       (pc->type == CT_MACRO_ELSE) ||
       (pc->type == CT_MACRO_CLOSE))
   {
      if (next->type == CT_PAREN_OPEN)
      {
         flag_parens(next, 0, CT_FPAREN_OPEN, pc->type, false);
      }
   }

   if ((pc->type == CT_DELETE) && (next->type == CT_TSQUARE))
   {
      set_chunk_parent(next, CT_DELETE);
   }

   /* Change CT_STAR to CT_PTR_TYPE or CT_ARITH or CT_DEREF */
   if (pc->type == CT_STAR || ((cpd.lang_flags & LANG_CPP) && (pc->type == CT_CARET)))
   {
      if (chunk_is_paren_close(next) || (next->type == CT_COMMA))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if ((cpd.lang_flags & LANG_OC) && (next->type == CT_STAR))
      {
         /* Change pointer-to-pointer types in OC_MSG_DECLs
          * from ARITH <===> DEREF to PTR_TYPE <===> PTR_TYPE */
         set_chunk_type(pc, CT_PTR_TYPE);
         set_chunk_parent(pc, prev->parent_type);

         set_chunk_type(next, CT_PTR_TYPE);
         set_chunk_parent(next, pc->parent_type);
      }
      else if ((pc->type == CT_STAR) && ((prev->type == CT_SIZEOF) || (prev->type == CT_DELETE)))
      {
         set_chunk_type(pc, CT_DEREF);
      }
      else if (((prev->type == CT_WORD) && chunk_ends_type(prev)) ||
               (prev->type == CT_DC_MEMBER) || (prev->type == CT_PTR_TYPE))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (next->type == CT_SQUARE_OPEN)
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (pc->type == CT_STAR)
      {
         /* most PCF_PUNCTUATOR chunks except a paren close would make this
          * a deref. A paren close may end a cast or may be part of a macro fcn.
          */
         set_chunk_type(pc,
                        ((prev->flags & PCF_PUNCTUATOR) &&
                         (!chunk_is_paren_close(prev) ||
                          (prev->parent_type == CT_MACRO_FUNC)) &&
                         (prev->type != CT_SQUARE_CLOSE) &&
                         (prev->type != CT_DC_MEMBER)) ? CT_DEREF : CT_ARITH);
      }
   }

   if (pc->type == CT_AMP)
   {
      if (prev->type == CT_DELETE)
      {
         set_chunk_type(pc, CT_ADDR);
      }
      else if (prev->type == CT_TYPE)
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else
      {
         set_chunk_type(pc, CT_ARITH);
         if (prev->type == CT_WORD)
         {
            tmp = chunk_get_prev_ncnl(prev);
            if ((tmp != NULL) &&
                (chunk_is_semicolon(tmp) ||
                 (tmp->type == CT_BRACE_OPEN) ||
                 (tmp->type == CT_QUALIFIER)))
            {
               set_chunk_type(prev, CT_TYPE);
               set_chunk_type(pc, CT_ADDR);
               next->flags |= PCF_VAR_1ST;
            }
         }
      }
   }

   if ((pc->type == CT_MINUS) ||
       (pc->type == CT_PLUS))
   {
      if ((prev->type == CT_POS) || (prev->type == CT_NEG))
      {
         set_chunk_type(pc, (pc->type == CT_MINUS) ? CT_NEG : CT_POS);
      }
      else if (prev->type == CT_OC_CLASS)
      {
         set_chunk_type(pc, (pc->type == CT_MINUS) ? CT_NEG : CT_POS);
      }
      else
      {
         set_chunk_type(pc, CT_ARITH);
      }
   }
}


/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
static void check_double_brace_init(chunk_t *bo1)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LJDBI, "%s: %d:%d", __func__, bo1->orig_line, bo1->orig_col);
   chunk_t *pc = chunk_get_prev_ncnl(bo1);
   if (chunk_is_paren_close(pc))
   {
      chunk_t *bo2 = chunk_get_next(bo1);
      if (chunk_is_token(bo2, CT_BRACE_OPEN))
      {
         /* found a potential double brace */
         chunk_t *bc2 = chunk_skip_to_match(bo2);
         chunk_t *bc1 = chunk_get_next(bc2);
         if (chunk_is_token(bc1, CT_BRACE_CLOSE))
         {
            LOG_FMT(LJDBI, " - end %d:%d\n", bc2->orig_line, bc2->orig_col);
            /* delete bo2 and bc1 */
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
}


/**
 * Change CT_INCDEC_AFTER + WORD to CT_INCDEC_BEFORE
 * Change number/word + CT_ADDR to CT_ARITH
 * Change number/word + CT_STAR to CT_ARITH
 * Change number/word + CT_NEG to CT_ARITH
 * Change word + ( to a CT_FUNCTION
 * Change struct/union/enum + CT_WORD => CT_TYPE
 * Force parens on return.
 *
 * TODO: This could be done earlier.
 *
 * Patterns detected:
 *   STRUCT/ENUM/UNION + WORD :: WORD => TYPE
 *   WORD + '('               :: WORD => FUNCTION
 */
void fix_symbols(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   chunk_t dummy;

   mark_define_expressions();

   bool is_java = (cpd.lang_flags & LANG_JAVA) != 0;
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((pc->type == CT_FUNC_WRAP) ||
          (pc->type == CT_TYPE_WRAP))
      {
         handle_wrap(pc);
      }

      if (pc->type == CT_ASSIGN)
      {
         mark_lvalue(pc);
      }

      if (is_java && (pc->type == CT_BRACE_OPEN))
      {
         check_double_brace_init(pc);
      }
   }

   pc = chunk_get_head();
   if (chunk_is_newline(pc) || chunk_is_comment(pc))
   {
      pc = chunk_get_next_ncnl(pc);
   }
   while (pc != NULL)
   {
      prev = chunk_get_prev_ncnl(pc, CNAV_PREPROC);
      if (prev == NULL)
      {
         prev = &dummy;
      }
      next = chunk_get_next_ncnl(pc, CNAV_PREPROC);
      if (next == NULL)
      {
         next = &dummy;
      }
      do_symbol_check(prev, pc, next);
      pc = chunk_get_next_ncnl(pc);
   }

   pawn_add_virtual_semicolons();
   process_returns();

   /**
    * 2nd pass - handle variable definitions
    * REVISIT: We need function params marked to do this (?)
    */
   pc = chunk_get_head();
   int square_level = -1;
   while (pc != NULL)
   {
      /* Can't have a variable definition inside [ ] */
      if (square_level < 0)
      {
         if (pc->type == CT_SQUARE_OPEN)
         {
            square_level = pc->level;
         }
      }
      else
      {
         if (pc->level <= square_level)
         {
            square_level = -1;
         }
      }

      /**
       * A variable definition is possible after at the start of a statement
       * that starts with: QUALIFIER, TYPE, or WORD
       */
      if ((square_level < 0) &&
          ((pc->flags & PCF_STMT_START) != 0) &&
          ((pc->type == CT_QUALIFIER) ||
           (pc->type == CT_TYPE) ||
           (pc->type == CT_WORD)) &&
          (pc->parent_type != CT_ENUM) &&
          ((pc->flags & PCF_IN_ENUM) == 0))
      {
         pc = fix_var_def(pc);
      }
      else
      {
         pc = chunk_get_next_ncnl(pc);
      }
   }
}


/* Just hit an assign. Go backwards until we hit an open brace/paren/square or
 * semicolon (TODO: other limiter?) and mark as a LValue.
 */
static void mark_lvalue(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev;

   if ((pc->flags & PCF_IN_PREPROC) != 0)
   {
      return;
   }

   for (prev = chunk_get_prev_ncnl(pc);
        prev != NULL;
        prev = chunk_get_prev_ncnl(prev))
   {
      if ((prev->level < pc->level) ||
          (prev->type == CT_ASSIGN) ||
          (prev->type == CT_COMMA) ||
          (prev->type == CT_BOOL) ||
          chunk_is_semicolon(prev) ||
          chunk_is_str(prev, "(", 1) ||
          chunk_is_str(prev, "{", 1) ||
          chunk_is_str(prev, "[", 1) ||
          (prev->flags & PCF_IN_PREPROC))
      {
         break;
      }
      prev->flags |= PCF_LVALUE;
      if ((prev->level == pc->level) && chunk_is_str(prev, "&", 1))
      {
         make_type(prev);
      }
   }
}


/**
 * Changes the return type to type and set the parent.
 *
 * @param pc the last chunk of the return type
 * @param parent_type CT_NONE (no change) or the new parent type
 */
static void mark_function_return_type(chunk_t *fname, chunk_t *start, c_token_t parent_type)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   if (pc)
   {
      /* Step backwards from pc and mark the parent of the return type */
      LOG_FMT(LFCNR, "%s: (backwards) return type for '%s' @ %d:%d", __func__,
              fname->text(), fname->orig_line, fname->orig_col);

      chunk_t *first = pc;
      while (pc)
      {
         if ((!chunk_is_type(pc) &&
              (pc->type != CT_OPERATOR) &&
              (pc->type != CT_WORD) &&
              (pc->type != CT_ADDR)) ||
             ((pc->flags & PCF_IN_PREPROC) != 0))
         {
            break;
         }
         if (!chunk_is_ptr_operator(pc))
         {
            first = pc;
         }
         pc = chunk_get_prev_ncnl(pc);
      }

      pc = first;
      while (pc)
      {
         LOG_FMT(LFCNR, " [%s|%s]", pc->text(), get_token_name(pc->type));

         if (parent_type != CT_NONE)
         {
            set_chunk_parent(pc, parent_type);
         }
         make_type(pc);
         if (pc == start)
         {
            break;
         }
         pc = chunk_get_next_ncnl(pc);
      }
      LOG_FMT(LFCNR, "\n");
   }
}


/**
 * Process a function type that is not in a typedef.
 * pc points to the first close paren.
 *
 * void (*func)(params);
 * const char * (*func)(params);
 * const char * (^func)(params);   -- Objective C
 *
 * @param pc   Points to the first closing paren
 * @return whether a function type was processed
 */
static bool mark_function_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTYPE, "%s: [%s] %s @ %d:%d\n",
           __func__, get_token_name(pc->type), pc->str.c_str(),
           pc->orig_line, pc->orig_col);

   int       star_count = 0;
   int       word_count = 0;
   chunk_t   *ptrcnk    = NULL;
   chunk_t   *varcnk    = NULL;
   chunk_t   *tmp;
   chunk_t   *apo;
   chunk_t   *apc;
   chunk_t   *aft;
   bool      anon = false;
   c_token_t pt, ptp;

   /* Scan backwards across the name, which can only be a word and single star */
   varcnk = chunk_get_prev_ncnl(pc);
   if (!chunk_is_word(varcnk))
   {
      if ((cpd.lang_flags & LANG_OC) && chunk_is_str(varcnk, "^", 1) &&
          chunk_is_paren_open(chunk_get_prev_ncnl(varcnk)))
      {
         /* anonymous ObjC block type -- RTYPE (^)(ARGS) */
         anon = true;
      }
      else
      {
         LOG_FMT(LFTYPE, "%s: not a word '%s' [%s] @ %d:%d\n",
                 __func__, varcnk->text(), get_token_name(varcnk->type),
                 varcnk->orig_line, varcnk->orig_col);
         goto nogo_exit;
      }
   }

   apo = chunk_get_next_ncnl(pc);
   apc = chunk_skip_to_match(apo);
   if (!chunk_is_paren_open(apo) || ((apc = chunk_skip_to_match(apo)) == NULL))
   {
      LOG_FMT(LFTYPE, "%s: not followed by parens\n", __func__);
      goto nogo_exit;
   }
   aft = chunk_get_next_ncnl(apc);
   if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      pt = CT_FUNC_DEF;
   }
   else if (chunk_is_token(aft, CT_SEMICOLON) ||
            chunk_is_token(aft, CT_ASSIGN))
   {
      pt = CT_FUNC_PROTO;
   }
   else
   {
      LOG_FMT(LFTYPE, "%s: not followed by '{' or ';'\n", __func__);
      goto nogo_exit;
   }
   ptp = (pc->flags & PCF_IN_TYPEDEF) ? CT_FUNC_TYPE : CT_FUNC_VAR;

   tmp = pc;
   while ((tmp = chunk_get_prev_ncnl(tmp)) != NULL)
   {
      LOG_FMT(LFTYPE, " -- [%s] %s on line %d, col %d",
              get_token_name(tmp->type), tmp->str.c_str(),
              tmp->orig_line, tmp->orig_col);

      if (chunk_is_star(tmp) || chunk_is_token(tmp, CT_PTR_TYPE) ||
          chunk_is_token(tmp, CT_CARET))
      {
         star_count++;
         ptrcnk = tmp;
         LOG_FMT(LFTYPE, " -- PTR_TYPE\n");
      }
      else if (chunk_is_word(tmp) ||
               (tmp->type == CT_WORD) ||
               (tmp->type == CT_TYPE))
      {
         word_count++;
         LOG_FMT(LFTYPE, " -- TYPE(%s)\n", tmp->text());
      }
      else if (tmp->type == CT_DC_MEMBER)
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
         LOG_FMT(LFTYPE, " --  unexpected token [%s] %s on line %d, col %d\n",
                 get_token_name(tmp->type), tmp->str.c_str(),
                 tmp->orig_line, tmp->orig_col);
         goto nogo_exit;
      }
   }

   if ((star_count > 1) ||
       (word_count > 1) ||
       ((star_count + word_count) == 0))
   {
      LOG_FMT(LFTYPE, "%s: bad counts word:%d, star:%d\n", __func__,
              word_count, star_count);
      goto nogo_exit;
   }

   /* make sure what appears before the first open paren can be a return type */
   if (!chunk_ends_type(chunk_get_prev_ncnl(tmp)))
   {
      goto nogo_exit;
   }

   if (ptrcnk)
   {
      set_chunk_type(ptrcnk, CT_PTR_TYPE);
   }
   if (!anon)
   {
      if (pc->flags & PCF_IN_TYPEDEF)
      {
         set_chunk_type(varcnk, CT_TYPE);
      }
      else
      {
         set_chunk_type(varcnk, CT_FUNC_VAR);
         varcnk->flags |= PCF_VAR_1ST_DEF;
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
      set_chunk_parent(aft, (aft->flags & PCF_IN_TYPEDEF) ? CT_TYPEDEF : CT_FUNC_VAR);
   }
   else if (chunk_is_token(aft, CT_BRACE_OPEN))
   {
      flag_parens(aft, 0, CT_NONE, pt, false);
   }

   /* Step backwards to the previous open paren and mark everything a
    */
   tmp = pc;
   while ((tmp = chunk_get_prev_ncnl(tmp)) != NULL)
   {
      LOG_FMT(LFTYPE, " ++ [%s] %s on line %d, col %d\n",
              get_token_name(tmp->type), tmp->str.c_str(),
              tmp->orig_line, tmp->orig_col);

      if (*tmp->str == '(')
      {
         if ((pc->flags & PCF_IN_TYPEDEF) == 0)
         {
            tmp->flags |= PCF_VAR_1ST_DEF;
         }
         set_chunk_type(tmp, CT_TPAREN_OPEN);
         set_chunk_parent(tmp, ptp);

         tmp = chunk_get_prev_ncnl(tmp);
         if (tmp != NULL)
         {
            if ((tmp->type == CT_FUNCTION) ||
                (tmp->type == CT_FUNC_CALL) ||
                (tmp->type == CT_FUNC_CALL_USER) ||
                (tmp->type == CT_FUNC_DEF) ||
                (tmp->type == CT_FUNC_PROTO))
            {
               set_chunk_type(tmp, CT_TYPE);
               tmp->flags &= ~PCF_VAR_1ST_DEF;
            }
         }
         mark_function_return_type(varcnk, tmp, ptp);
         break;
      }
   }
   return true;

nogo_exit:
   tmp = chunk_get_next_ncnl(pc);
   if (chunk_is_paren_open(tmp))
   {
      LOG_FMT(LFTYPE, "%s:%d setting FUNC_CALL on %d:%d\n", __func__, __LINE__,
              tmp->orig_line, tmp->orig_col);
      flag_parens(tmp, 0, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
   }
   return false;
}


static void process_returns(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if ((pc->type != CT_RETURN) || (pc->flags & PCF_IN_PREPROC))
      {
         pc = chunk_get_next_type(pc, CT_RETURN, -1);
         continue;
      }

      pc = process_return(pc);
   }
}


/**
 * Processes a return statement, labeling the parens and marking the parent.
 * May remove or add parens around the return statement
 *
 * @param pc   Pointer to the return chunk
 */
static chunk_t *process_return(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *temp;
   chunk_t *semi;
   chunk_t *cpar;
   chunk_t chunk;

   /* grab next and bail if it is a semicolon */
   next = chunk_get_next_ncnl(pc);
   if ((next == NULL) || chunk_is_semicolon(next))
   {
      return(next);
   }

   if (cpd.settings[UO_nl_return_expr].a != AV_IGNORE)
   {
      newline_iarf(pc, cpd.settings[UO_nl_return_expr].a);
   }

   if (next->type == CT_PAREN_OPEN)
   {
      /* See if the return is fully paren'd */
      cpar = chunk_get_next_type(next, CT_PAREN_CLOSE, next->level);
      semi = chunk_get_next_ncnl(cpar);
      if (chunk_is_semicolon(semi))
      {
         if (cpd.settings[UO_mod_paren_on_return].a == AV_REMOVE)
         {
            LOG_FMT(LRETURN, "%s: removing parens on line %d\n",
                    __func__, pc->orig_line);

            /* lower the level of everything */
            for (temp = next; temp != cpar; temp = chunk_get_next(temp))
            {
               temp->level--;
            }

            /* delete the parens */
            chunk_del(next);
            chunk_del(cpar);

            /* back up the semicolon */
            semi->column--;
            semi->orig_col--;
            semi->orig_col_end--;
         }
         else
         {
            LOG_FMT(LRETURN, "%s: keeping parens on line %d\n",
                    __func__, pc->orig_line);

            /* mark & keep them */
            set_chunk_parent(next, CT_RETURN);
            set_chunk_parent(cpar, CT_RETURN);
         }
         return(semi);
      }
   }

   /* We don't have a fully paren'd return. Should we add some? */
   if ((cpd.settings[UO_mod_paren_on_return].a & AV_ADD) == 0)
   {
      return(next);
   }

   /* find the next semicolon on the same level */
   semi = next;
   while ((semi = chunk_get_next(semi)) != NULL)
   {
      if ((chunk_is_semicolon(semi) && (pc->level == semi->level)) ||
          (semi->level < pc->level))
      {
         break;
      }
   }
   if (chunk_is_semicolon(semi) && (pc->level == semi->level))
   {
      /* add the parens */
      chunk.type        = CT_PAREN_OPEN;
      chunk.str         = "(";
      chunk.level       = pc->level;
      chunk.brace_level = pc->brace_level;
      chunk.orig_line   = pc->orig_line;
      chunk.parent_type = CT_RETURN;
      chunk.flags       = pc->flags & PCF_COPY_FLAGS;
      chunk_add_before(&chunk, next);

      chunk.type      = CT_PAREN_CLOSE;
      chunk.str       = ")";
      chunk.orig_line = semi->orig_line;
      cpar            = chunk_add_before(&chunk, semi);

      LOG_FMT(LRETURN, "%s: added parens on line %d\n",
              __func__, pc->orig_line);

      for (temp = next; temp != cpar; temp = chunk_get_next(temp))
      {
         temp->level++;
      }
   }
   return(semi);
}


static bool is_ucase_str(const char *str, int len)
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
   return((pc != NULL) &&
          ((pc->parent_type == CT_OC_BLOCK_TYPE) ||
           (pc->parent_type == CT_OC_BLOCK_EXPR) ||
           (pc->parent_type == CT_OC_BLOCK_ARG) ||
           (pc->parent_type == CT_OC_BLOCK) ||
           (pc->type == CT_OC_BLOCK_CARET) ||
           (pc->next && pc->next->type == CT_OC_BLOCK_CARET) ||
           (pc->prev && pc->prev->type == CT_OC_BLOCK_CARET)));
}


/**
 * Checks to see if the current paren is part of a cast.
 * We already verified that this doesn't follow function, TYPE, IF, FOR,
 * SWITCH, or WHILE and is followed by WORD, TYPE, STRUCT, ENUM, or UNION.
 *
 * @param start   Pointer to the open paren
 */
static void fix_casts(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc;
   chunk_t    *prev;
   chunk_t    *first;
   chunk_t    *after;
   chunk_t    *last = NULL;
   chunk_t    *paren_close;
   const char *verb       = "likely";
   const char *detail     = "";
   int        count       = 0;
   int        word_count  = 0;
   int        word_consec = 0;
   bool       nope;
   bool       doubtful_cast = false;


   LOG_FMT(LCASTS, "%s:line %d, col %d:", __func__, start->orig_line, start->orig_col);

   prev = chunk_get_prev_ncnl(start);
   if ((prev != NULL) && (prev->type == CT_PP_DEFINED))
   {
      LOG_FMT(LCASTS, " -- not a cast - after defined\n");
      return;
   }

   /* Make sure there is only WORD, TYPE, and '*' or '^' before the close paren */
   pc    = chunk_get_next_ncnl(start);
   first = pc;
   while ((pc != NULL) && (chunk_is_type(pc) ||
                           (pc->type == CT_WORD) ||
                           (pc->type == CT_QUALIFIER) ||
                           (pc->type == CT_DC_MEMBER) ||
                           (pc->type == CT_STAR) ||
                           (pc->type == CT_CARET) ||
                           (pc->type == CT_TSQUARE) ||
                           (pc->type == CT_AMP)))
   {
      LOG_FMT(LCASTS, " [%s]", get_token_name(pc->type));

      if (pc->type == CT_WORD)
      {
         word_count++;
         word_consec++;
      }
      else if (pc->type == CT_DC_MEMBER)
      {
         word_count--;
      }
      else
      {
         word_consec = 0;
      }

      last = pc;
      pc   = chunk_get_next_ncnl(pc);
      count++;
   }

   if ((pc == NULL) || (pc->type != CT_PAREN_CLOSE) || (prev->type == CT_OC_CLASS))
   {
      LOG_FMT(LCASTS, " -- not a cast, hit [%s]\n",
              pc == NULL ? "NULL"  : get_token_name(pc->type));
      return;
   }

   if (word_count > 1)
   {
      LOG_FMT(LCASTS, " -- too many words: %d\n", word_count);
      return;
   }
   paren_close = pc;

   /* If last is a type or star/caret, we have a cast for sure */
   if ((last->type == CT_STAR) ||
       (last->type == CT_CARET) ||
       (last->type == CT_PTR_TYPE) ||
       (last->type == CT_TYPE))
   {
      verb = "for sure";
   }
   else if (count == 1)
   {
      /**
       * We are on a potential cast of the form "(word)".
       * We don't know if the word is a type. So lets guess based on some
       * simple rules:
       *  - if all caps, likely a type
       *  - if it ends in _t, likely a type
       *  - if it's objective-c and the type is id, likely valid
       */
      verb = "guessed";
      if ((last->len() > 3) &&
          (last->str[last->len() - 2] == '_') &&
          (last->str[last->len() - 1] == 't'))
      {
         detail = " -- '_t'";
      }
      else if (is_ucase_str(last->text(), last->len()))
      {
         detail = " -- upper case";
      }
      else if ((cpd.lang_flags & LANG_OC) && chunk_is_str(last, "id", 2))
      {
         detail = " -- Objective-C id";
      }
      else
      {
         /* If we can't tell for sure whether this is a cast, decide against it */
         detail        = " -- mixed case";
         doubtful_cast = true;
      }

      /**
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
      } while ((after != NULL) && (after->type == CT_PAREN_OPEN));

      if (after == NULL)
      {
         LOG_FMT(LCASTS, " -- not a cast - hit NULL\n");
         return;
      }

      nope = false;
      if (chunk_is_ptr_operator(pc))
      {
         /* star (*) and addr (&) are ambiguous */
         if ((after->type == CT_NUMBER_FP) ||
             (after->type == CT_NUMBER) ||
             (after->type == CT_STRING) ||
             doubtful_cast)
         {
            nope = true;
         }
      }
      else if (pc->type == CT_MINUS)
      {
         /* (UINT8)-1 or (foo)-1 or (FOO)-'a' */
         if ((after->type == CT_STRING) || doubtful_cast)
         {
            nope = true;
         }
      }
      else if (pc->type == CT_PLUS)
      {
         /* (UINT8)+1 or (foo)+1 */
         if (((after->type != CT_NUMBER) &&
              (after->type != CT_NUMBER_FP)) || doubtful_cast)
         {
            nope = true;
         }
      }
      else if ((pc->type != CT_NUMBER_FP) &&
               (pc->type != CT_NUMBER) &&
               (pc->type != CT_WORD) &&
               (pc->type != CT_THIS) &&
               (pc->type != CT_TYPE) &&
               (pc->type != CT_PAREN_OPEN) &&
               (pc->type != CT_STRING) &&
               (pc->type != CT_SIZEOF) &&
               (pc->type != CT_FUNC_CALL) &&
               (pc->type != CT_FUNC_CALL_USER) &&
               (pc->type != CT_FUNCTION) &&
               (pc->type != CT_BRACE_OPEN) &&
               (!((pc->type == CT_SQUARE_OPEN) && (cpd.lang_flags & LANG_OC))))
      {
         LOG_FMT(LCASTS, " -- not a cast - followed by '%s' %s\n",
                 pc->str.c_str(), get_token_name(pc->type));
         return;
      }

      if (nope)
      {
         LOG_FMT(LCASTS, " -- not a cast - '%s' followed by %s\n",
                 pc->str.c_str(), get_token_name(after->type));
         return;
      }
   }

   /* if the 'cast' is followed by a semicolon, comma or close paren, it isn't */
   pc = chunk_get_next_ncnl(paren_close);
   if (chunk_is_semicolon(pc) || chunk_is_token(pc, CT_COMMA) || chunk_is_paren_close(pc))
   {
      LOG_FMT(LCASTS, " -- not a cast - followed by %s\n", get_token_name(pc->type));
      return;
   }

   set_chunk_parent(start, CT_C_CAST);
   set_chunk_parent(paren_close, CT_C_CAST);

   LOG_FMT(LCASTS, " -- %s c-cast: (", verb);

   for (pc = first; pc != paren_close; pc = chunk_get_next_ncnl(pc))
   {
      set_chunk_parent(pc, CT_C_CAST);
      make_type(pc);
      LOG_FMT(LCASTS, " %s", pc->str.c_str());
   }
   LOG_FMT(LCASTS, " )%s\n", detail);

   /* Mark the next item as an expression start */
   pc = chunk_get_next_ncnl(paren_close);
   if (pc != NULL)
   {
      pc->flags |= PCF_EXPR_START;
      if (chunk_is_opening_brace(pc))
      {
         set_paren_parent(pc, start->parent_type);
      }
   }
}


/**
 * CT_TYPE_CAST follows this pattern:
 * dynamic_cast<...>(...)
 *
 * Mark everything between the <> as a type and set the paren parent
 */
static void fix_type_cast(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;

   pc = chunk_get_next_ncnl(start);
   if ((pc == NULL) || (pc->type != CT_ANGLE_OPEN))
   {
      return;
   }

   while (((pc = chunk_get_next_ncnl(pc)) != NULL) &&
          (pc->level >= start->level))
   {
      if ((pc->level == start->level) && (pc->type == CT_ANGLE_CLOSE))
      {
         pc = chunk_get_next_ncnl(pc);
         if (chunk_is_str(pc, "(", 1))
         {
            set_paren_parent(pc, CT_TYPE_CAST);
         }
         return;
      }
      make_type(pc);
   }
}


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
static void fix_enum_struct_union(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *next;
   chunk_t *prev        = NULL;
   int     flags        = PCF_VAR_1ST_DEF;
   int     in_fcn_paren = pc->flags & PCF_IN_FCN_DEF;

   /* Make sure this wasn't a cast */
   if (pc->parent_type == CT_C_CAST)
   {
      return;
   }

   /* the next item is either a type or open brace */
   next = chunk_get_next_ncnl(pc);
   if (next && (next->type == CT_ENUM_CLASS))
   {
      next = chunk_get_next_ncnl(next);
   }
   if (next && (next->type == CT_TYPE))
   {
      set_chunk_parent(next, pc->type);
      prev = next;
      next = chunk_get_next_ncnl(next);

      /* next up is either a colon, open brace, or open paren (pawn) */
      if (!next)
      {
         return;
      }
      else if (((cpd.lang_flags & LANG_PAWN) != 0) &&
               (next->type == CT_PAREN_OPEN))
      {
         next = set_paren_parent(next, CT_ENUM);
      }
      else if ((pc->type == CT_ENUM) && (next->type == CT_COLON))
      {
         /* enum TYPE : INT_TYPE { */
         next = chunk_get_next_ncnl(next);
         if (next)
         {
            make_type(next);
            next = chunk_get_next_ncnl(next);
         }
      }
   }
   if (next && (next->type == CT_BRACE_OPEN))
   {
      flag_parens(next, (pc->type == CT_ENUM) ? PCF_IN_ENUM : PCF_IN_STRUCT,
                  CT_NONE, CT_NONE, false);

      if ((pc->type == CT_UNION) || (pc->type == CT_STRUCT))
      {
         mark_struct_union_body(next);
      }

      /* Skip to the closing brace */
      set_chunk_parent(next, pc->type);
      next   = chunk_get_next_type(next, CT_BRACE_CLOSE, pc->level);
      flags |= PCF_VAR_INLINE;
      if (next != NULL)
      {
         set_chunk_parent(next, pc->type);
         next = chunk_get_next_ncnl(next);
      }
      prev = NULL;
   }
   /* reset var name parent type */
   else if (next && prev)
   {
      set_chunk_parent(prev, CT_NONE);
   }

   if ((next == NULL) || (next->type == CT_PAREN_CLOSE))
   {
      return;
   }

   if (!chunk_is_semicolon(next))
   {
      /* Pawn does not require a semicolon after an enum */
      if (cpd.lang_flags & LANG_PAWN)
      {
         return;
      }

      /* D does not require a semicolon after an enum, but we add one to make
       * other code happy.
       */
      if (cpd.lang_flags & LANG_D)
      {
         next = pawn_add_vsemi_after(chunk_get_prev_ncnl(next));
      }
   }

   /* We are either pointing to a ';' or a variable */
   while ((next != NULL) && !chunk_is_semicolon(next) &&
          (next->type != CT_ASSIGN) &&
          ((in_fcn_paren ^ (next->flags & PCF_IN_FCN_DEF)) == 0))
   {
      if (next->level == pc->level)
      {
         if (next->type == CT_WORD)
         {
            next->flags |= flags;
            flags       &= ~PCF_VAR_1ST; /* clear the first flag for the next items */
         }

         if (next->type == CT_STAR || ((cpd.lang_flags & LANG_CPP) && (next->type == CT_CARET)))
         {
            set_chunk_type(next, CT_PTR_TYPE);
         }

         /* If we hit a comma in a function param, we are done */
         if (((next->type == CT_COMMA) ||
              (next->type == CT_FPAREN_CLOSE)) &&
             ((next->flags & (PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)) != 0))
         {
            return;
         }
      }

      next = chunk_get_next_ncnl(next);
   }

   if (next && !prev && (next->type == CT_SEMICOLON))
   {
      set_chunk_parent(next, pc->type);
   }
}


/**
 * We are on a typedef.
 * If the next word is not enum/union/struct, then the last word before the
 * next ',' or ';' or '__attribute__' is a type.
 *
 * typedef [type...] [*] type [, [*]type] ;
 * typedef <return type>([*]func)(params);
 * typedef <return type>func(params);
 * typedef <enum/struct/union> [type] [*] type [, [*]type] ;
 * typedef <enum/struct/union> [type] { ... } [*] type [, [*]type] ;
 */
static void fix_typedef(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t   *next;
   chunk_t   *the_type = NULL;
   chunk_t   *open_paren;
   chunk_t   *last_op = NULL;
   c_token_t tag;

   LOG_FMT(LTYPEDEF, "%s: typedef @ %d:%d\n", __func__, start->orig_line, start->orig_col);

   /* Mark everything in the typedef and scan for ")(", which makes it a
    * function type
    */
   next = start;
   while (((next = chunk_get_next_ncnl(next, CNAV_PREPROC)) != NULL) &&
          (next->level >= start->level))
   {
      next->flags |= PCF_IN_TYPEDEF;
      if (start->level == next->level)
      {
         if (chunk_is_semicolon(next))
         {
            set_chunk_parent(next, CT_TYPEDEF);
            break;
         }
         if (next->type == CT_ATTRIBUTE)
         {
            break;
         }
         if ((cpd.lang_flags & LANG_D) && (next->type == CT_ASSIGN))
         {
            set_chunk_parent(next, CT_TYPEDEF);
            break;
         }
         make_type(next);
         if (next->type == CT_TYPE)
         {
            the_type = next;
         }
         next->flags &= ~PCF_VAR_1ST_DEF;
         if (*next->str == '(')
         {
            last_op = next;
         }
      }
   }

   /* avoid interpreting typedef NS_ENUM (NSInteger, MyEnum) as a function def */
   if (last_op && !((cpd.lang_flags & LANG_OC) &&
                    (last_op->parent_type == CT_ENUM)))
   {
      flag_parens(last_op, 0, CT_FPAREN_OPEN, CT_TYPEDEF, false);
      fix_fcn_def_params(last_op);

      open_paren = NULL;
      the_type   = chunk_get_prev_ncnl(last_op, CNAV_PREPROC);
      if (chunk_is_paren_close(the_type))
      {
         open_paren = chunk_skip_to_match_rev(the_type);
         mark_function_type(the_type);
         the_type = chunk_get_prev_ncnl(the_type, CNAV_PREPROC);
      }
      else
      {
         /* must be: "typedef <return type>func(params);" */
         set_chunk_type(the_type, CT_FUNC_TYPE);
      }
      set_chunk_parent(the_type, CT_TYPEDEF);

      LOG_FMT(LTYPEDEF, "%s: fcn typedef [%s] on line %d\n", __func__,
              the_type->text(), the_type->orig_line);

      /* If we are aligning on the open paren, grab that instead */
      if (open_paren && (cpd.settings[UO_align_typedef_func].n == 1))
      {
         the_type = open_paren;
      }
      if (cpd.settings[UO_align_typedef_func].n != 0)
      {
         LOG_FMT(LTYPEDEF, "%s:  -- align anchor on [%s] @ %d:%d\n", __func__,
                 the_type->text(), the_type->orig_line, the_type->orig_col);
         the_type->flags |= PCF_ANCHOR;
      }

      /* already did everything we need to do */
      return;
   }

   /**
    * Skip over enum/struct/union stuff, as we know it isn't a return type
    * for a function type
    */
   next = chunk_get_next_ncnl(start, CNAV_PREPROC);
   if ((next->type != CT_ENUM) &&
       (next->type != CT_STRUCT) &&
       (next->type != CT_UNION))
   {
      if (the_type != NULL)
      {
         /* We have just a regular typedef */
         LOG_FMT(LTYPEDEF, "%s: regular typedef [%s] on line %d\n", __func__,
                 the_type->str.c_str(), the_type->orig_line);
         the_type->flags |= PCF_ANCHOR;
      }
      return;
   }

   /* We have a struct/union/enum type, set the parent */
   tag = next->type;

   /* the next item should be either a type or { */
   next = chunk_get_next_ncnl(next, CNAV_PREPROC);
   if (next->type == CT_TYPE)
   {
      next = chunk_get_next_ncnl(next, CNAV_PREPROC);
   }
   if (next->type == CT_BRACE_OPEN)
   {
      set_chunk_parent(next, tag);
      /* Skip to the closing brace */
      next = chunk_get_next_type(next, CT_BRACE_CLOSE, next->level, CNAV_PREPROC);
      if (next != NULL)
      {
         set_chunk_parent(next, tag);
      }
   }

   if (the_type != NULL)
   {
      LOG_FMT(LTYPEDEF, "%s: %s typedef [%s] on line %d\n",
              __func__, get_token_name(tag), the_type->str.c_str(), the_type->orig_line);
      the_type->flags |= PCF_ANCHOR;
   }
}


/**
 * Examines the whole file and changes CT_COLON to
 * CT_Q_COLON, CT_LABEL_COLON, or CT_CASE_COLON.
 * It also changes the CT_WORD before CT_LABEL_COLON into CT_LABEL.
 */
void combine_labels(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *cur;
   chunk_t *prev;
   chunk_t *next;
   chunk_t *tmp;
   bool    hit_case       = false;
   bool    hit_class      = false;

   // need a stack to handle nesting inside of OC messages, which reset the scope
   std::stack<int> question_counts;
   question_counts.push(0);

   prev = chunk_get_head();
   cur  = chunk_get_next_nc(prev);
   next = chunk_get_next_nc(cur);

   /* unlikely that the file will start with a label... */
   while (next != NULL)
   {
      if (!(next->flags & PCF_IN_OC_MSG) && /* filter OC case of [self class] msg send */
          ((next->type == CT_CLASS) ||
           (next->type == CT_OC_CLASS) ||
           (next->type == CT_TEMPLATE)))
      {
         hit_class = true;
      }
      if (chunk_is_semicolon(next) || (next->type == CT_BRACE_OPEN))
      {
         hit_class = false;
      }

      if (prev->type == CT_SQUARE_OPEN && prev->parent_type == CT_OC_MSG)
      {
         question_counts.push(0);
      }
      else if (next->type == CT_SQUARE_CLOSE && next->parent_type == CT_OC_MSG)
      {
         question_counts.pop();
      }

      if (next->type == CT_QUESTION)
      {
         ++question_counts.top();
      }
      else if (next->type == CT_CASE)
      {
         if (cur->type == CT_GOTO)
         {
            /* handle "goto case x;" */
            set_chunk_type(next, CT_QUALIFIER);
         }
         else
         {
            hit_case = true;
         }
      }
      else if ((next->type == CT_COLON) ||
               ((question_counts.top() > 0) && (next->type == CT_OC_COLON)))
      {
         if (cur->type == CT_DEFAULT)
         {
            set_chunk_type(cur, CT_CASE);
            hit_case = true;
         }
         if (question_counts.top() > 0)
         {
            set_chunk_type(next, CT_COND_COLON);
            --question_counts.top();
         }
         else if (hit_case)
         {
            hit_case = false;
            set_chunk_type(next, CT_CASE_COLON);
            tmp = chunk_get_next_ncnl(next);
            if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
            {
               set_chunk_parent(tmp, CT_CASE);
               tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);
               if (tmp != NULL)
               {
                  set_chunk_parent(tmp, CT_CASE);
               }
            }
         }
         else
         {
            chunk_t *nextprev = chunk_get_prev_ncnl(next);

            if ((cpd.lang_flags & LANG_PAWN) != 0)
            {
               if ((cur->type == CT_WORD) ||
                   (cur->type == CT_BRACE_CLOSE))
               {
                  c_token_t new_type = CT_TAG;

                  tmp = chunk_get_next_nc(next);
                  if (chunk_is_newline(prev) && chunk_is_newline(tmp))
                  {
                     new_type = CT_LABEL;
                     set_chunk_type(next, CT_LABEL_COLON);
                  }
                  else
                  {
                     set_chunk_type(next, CT_TAG_COLON);
                  }
                  if (cur->type == CT_WORD)
                  {
                     set_chunk_type(cur, new_type);
                  }
               }
            }
            else if (next->flags & PCF_IN_ARRAY_ASSIGN)
            {
               set_chunk_type(next, CT_D_ARRAY_COLON);
            }
            else if (next->flags & PCF_IN_FOR)
            {
               set_chunk_type(next, CT_FOR_COLON);
            }
            else if (next->flags & PCF_OC_BOXED)
            {
               set_chunk_type(next, CT_OC_DICT_COLON);
            }
            else if (cur->type == CT_WORD)
            {
               tmp = chunk_get_next_nc(next, CNAV_PREPROC);
               if (next->flags & PCF_IN_FCN_CALL)
               {
                  /* Must be a macro thingy, assume some sort of label */
                  set_chunk_type(next, CT_LABEL_COLON);
               }
               else if ((tmp == NULL) || (tmp->type != CT_NUMBER && tmp->type != CT_SIZEOF))
               {
                  /* the CT_SIZEOF isn't great - test 31720 happens to use a sizeof expr, but this really should be able to handle any constant expr */
                  set_chunk_type(cur, CT_LABEL);
                  set_chunk_type(next, CT_LABEL_COLON);
               }
               else if (next->flags & (PCF_IN_STRUCT | PCF_IN_CLASS | PCF_IN_TYPEDEF))
               {
                  set_chunk_type(next, CT_BIT_COLON);

                  tmp = chunk_get_next(next);
                  while ((tmp = chunk_get_next(tmp)) != NULL)
                  {
                     if (tmp->type == CT_SEMICOLON)
                     {
                        break;
                     }
                     if (tmp->type == CT_COLON)
                     {
                        set_chunk_type(tmp, CT_BIT_COLON);
                     }
                  }
               }
            }
            else if (nextprev->type == CT_FPAREN_CLOSE)
            {
               /* it's a class colon */
               set_chunk_type(next, CT_CLASS_COLON);
            }
            else if (next->level > next->brace_level)
            {
               /* ignore it, as it is inside a paren */
            }
            else if (cur->type == CT_TYPE)
            {
               set_chunk_type(next, CT_BIT_COLON);
            }
            else if ((cur->type == CT_ENUM) ||
                     (cur->type == CT_PRIVATE) ||
                     (cur->type == CT_QUALIFIER) ||
                     (cur->parent_type == CT_ALIGN))
            {
               /* ignore it - bit field, align or public/private, etc */
            }
            else if ((cur->type == CT_ANGLE_CLOSE) || hit_class)
            {
               /* ignore it - template thingy */
            }
            else if (cur->parent_type == CT_SQL_EXEC)
            {
               /* ignore it - SQL variable name */
            }
            else if (next->parent_type == CT_ASSERT)
            {
               /* ignore it - Java assert thing */
            }
            else
            {
               tmp = chunk_get_next_ncnl(next);
               if ((tmp != NULL) && ((tmp->type == CT_BASE) ||
                                     (tmp->type == CT_THIS)))
               {
                  /* ignore it, as it is a C# base thingy */
               }
               else
               {
                  LOG_FMT(LWARN, "%s:%d unexpected colon in col %d n-parent=%s c-parent=%s l=%d bl=%d\n",
                          cpd.filename, next->orig_line, next->orig_col,
                          get_token_name(next->parent_type),
                          get_token_name(cur->parent_type),
                          next->level, next->brace_level);
                  cpd.error_count++;
               }
            }
         }
      }
      prev = cur;
      cur  = next;
      next = chunk_get_next_nc(cur);
   }
}


static void mark_variable_stack(ChunkStack& cs, log_sev_t sev)
{
   LOG_FUNC_ENTRY();
   chunk_t *var_name;
   chunk_t *word_type;
   int     word_cnt = 0;

   /* throw out the last word and mark the rest */
   var_name = cs.Pop_Back();
   if (var_name && (var_name->prev->type == CT_DC_MEMBER))
   {
      cs.Push_Back(var_name);
   }

   if (var_name != NULL)
   {
      LOG_FMT(LFCNP, "%s: parameter on line %d :",
              __func__, var_name->orig_line);

      while ((word_type = cs.Pop_Back()) != NULL)
      {
         if ((word_type->type == CT_WORD) || (word_type->type == CT_TYPE))
         {
            LOG_FMT(LFCNP, " <%s>", word_type->str.c_str());

            set_chunk_type(word_type, CT_TYPE);
            word_type->flags |= PCF_VAR_TYPE;
         }
         word_cnt++;
      }

      if (var_name->type == CT_WORD)
      {
         if (word_cnt)
         {
            LOG_FMT(LFCNP, " [%s]\n", var_name->str.c_str());
            var_name->flags |= PCF_VAR_DEF;
         }
         else
         {
            LOG_FMT(LFCNP, " <%s>\n", var_name->str.c_str());
            set_chunk_type(var_name, CT_TYPE);
            var_name->flags |= PCF_VAR_TYPE;
         }
      }
   }
}


/**
 * Simply change any STAR to PTR_TYPE and WORD to TYPE
 *
 * @param start points to the open paren
 */
static void fix_fcn_def_params(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFCNP, "%s: %s [%s] on line %d, level %d\n",
           __func__, start->str.c_str(), get_token_name(start->type), start->orig_line, start->level);

   while ((start != NULL) && !chunk_is_paren_open(start))
   {
      start = chunk_get_next_ncnl(start);
   }

   assert((start->len() == 1) && (start->str[0] == '('));

   ChunkStack cs;

   int level = start->level + 1;

   chunk_t *pc = start;
   while ((pc = chunk_get_next_ncnl(pc)) != NULL)
   {
      if (((start->len() == 1) && (start->str[0] == ')')) ||
          (pc->level < level))
      {
         LOG_FMT(LFCNP, "%s: bailed on %s on line %d\n", __func__, pc->str.c_str(), pc->orig_line);
         break;
      }

      LOG_FMT(LFCNP, "%s: %s %s on line %d, level %d\n", __func__,
              (pc->level > level) ? "skipping" : "looking at",
              pc->str.c_str(), pc->orig_line, pc->level);

      if (pc->level > level)
      {
         continue;
      }
      if (chunk_is_star(pc) || chunk_is_msref(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
         cs.Push_Back(pc);
      }
      else if ((pc->type == CT_AMP) ||
               ((cpd.lang_flags & LANG_CPP) && chunk_is_str(pc, "&&", 2)))
      {
         set_chunk_type(pc, CT_BYREF);
         cs.Push_Back(pc);
      }
      else if (pc->type == CT_TYPE_WRAP)
      {
         cs.Push_Back(pc);
      }
      else if ((pc->type == CT_WORD) || (pc->type == CT_TYPE))
      {
         cs.Push_Back(pc);
      }
      else if ((pc->type == CT_COMMA) || (pc->type == CT_ASSIGN))
      {
         mark_variable_stack(cs, LFCNP);
         if (pc->type == CT_ASSIGN)
         {
            /* Mark assignment for default param spacing */
            set_chunk_parent(pc, CT_FUNC_PROTO);
         }
      }
   }
   mark_variable_stack(cs, LFCNP);
}


/**
 * Skips to the start of the next statement.
 */
static chunk_t *skip_to_next_statement(chunk_t *pc)
{
   while ((pc != NULL) && !chunk_is_semicolon(pc) &&
          (pc->type != CT_BRACE_OPEN) &&
          (pc->type != CT_BRACE_CLOSE))
   {
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


/**
 * We are on the start of a sequence that could be a var def
 *  - FPAREN_OPEN (parent == CT_FOR)
 *  - BRACE_OPEN
 *  - SEMICOLON
 *
 */
static chunk_t *fix_var_def(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *pc = start;
   chunk_t    *end;
   chunk_t    *tmp_pc;
   ChunkStack cs;
   int        idx, ref_idx;

   LOG_FMT(LFVD, "%s: start[%d:%d]", __func__, pc->orig_line, pc->orig_col);

   /* Scan for words and types and stars oh my! */
   while ((pc != NULL) &&
          ((pc->type == CT_TYPE) ||
           (pc->type == CT_WORD) ||
           (pc->type == CT_QUALIFIER) ||
           (pc->type == CT_DC_MEMBER) ||
           (pc->type == CT_MEMBER) ||
           chunk_is_ptr_operator(pc)))
   {
      LOG_FMT(LFVD, " %s[%s]", pc->str.c_str(), get_token_name(pc->type));
      cs.Push_Back(pc);
      pc = chunk_get_next_ncnl(pc);

      /* Skip templates and attributes */
      pc = skip_template_next(pc);
      pc = skip_attribute_next(pc);
      if (cpd.lang_flags & LANG_JAVA)
      {
         pc = skip_tsquare_next(pc);
      }
   }
   end = pc;

   LOG_FMT(LFVD, " end=[%s]\n", (end != NULL) ? get_token_name(end->type) : "NULL");

   if (end == NULL)
   {
      return(NULL);
   }

   /* Function defs are handled elsewhere */
   if ((cs.Len() <= 1) ||
       (end->type == CT_FUNC_DEF) ||
       (end->type == CT_FUNC_PROTO) ||
       (end->type == CT_FUNC_CLASS_DEF) ||
       (end->type == CT_FUNC_CLASS_PROTO) ||
       (end->type == CT_OPERATOR))
   {
      return(skip_to_next_statement(end));
   }

   /* ref_idx points to the alignable part of the var def */
   ref_idx = cs.Len() - 1;

   /* Check for the '::' stuff: "char *Engine::name" */
   if ((cs.Len() >= 3) &&
       ((cs.Get(cs.Len() - 2)->m_pc->type == CT_MEMBER) ||
        (cs.Get(cs.Len() - 2)->m_pc->type == CT_DC_MEMBER)))
   {
      idx = cs.Len() - 2;
      while (idx > 0)
      {
         tmp_pc = cs.Get(idx)->m_pc;
         if ((tmp_pc->type != CT_DC_MEMBER) &&
             (tmp_pc->type != CT_MEMBER))
         {
            break;
         }
         idx--;
         tmp_pc = cs.Get(idx)->m_pc;
         if ((tmp_pc->type != CT_WORD) &&
             (tmp_pc->type != CT_TYPE))
         {
            break;
         }
         make_type(tmp_pc);
         idx--;
      }
      ref_idx = idx + 1;
   }
   tmp_pc = cs.Get(ref_idx)->m_pc;
   LOG_FMT(LFVD, " ref_idx(%d) => %s\n", ref_idx, tmp_pc->str.c_str());

   /* No type part found! */
   if (ref_idx <= 0)
   {
      return(skip_to_next_statement(end));
   }

   LOG_FMT(LFVD2, "%s:%d TYPE : ", __func__, start->orig_line);
   for (idx = 0; idx < cs.Len() - 1; idx++)
   {
      tmp_pc = cs.Get(idx)->m_pc;
      make_type(tmp_pc);
      tmp_pc->flags |= PCF_VAR_TYPE;
      LOG_FMT(LFVD2, " %s[%s]", tmp_pc->str.c_str(), get_token_name(tmp_pc->type));
   }
   LOG_FMT(LFVD2, "\n");

   /**
    * OK we have two or more items, mark types up to the end.
    */
   mark_variable_definition(cs.Get(cs.Len() - 1)->m_pc);
   if (end->type == CT_COMMA)
   {
      return(chunk_get_next_ncnl(end));
   }
   return(skip_to_next_statement(end));
}


/**
 * Skips everything until a comma or semicolon at the same level.
 * Returns the semicolon, comma, or close brace/paren or NULL.
 */
static chunk_t *skip_expression(chunk_t *start)
{
   chunk_t *pc = start;

   while ((pc != NULL) && (pc->level >= start->level))
   {
      if ((pc->level == start->level) &&
          (chunk_is_semicolon(pc) || (pc->type == CT_COMMA)))
      {
         return(pc);
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


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
static chunk_t *mark_variable_definition(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc   = start;
   int     flags = PCF_VAR_1ST_DEF;

   if (start == NULL)
   {
      return(NULL);
   }

   LOG_FMT(LVARDEF, "%s: line %d, col %d '%s' type %s\n",
           __func__,
           pc->orig_line, pc->orig_col, pc->str.c_str(),
           get_token_name(pc->type));

   pc = start;
   while ((pc != NULL) && !chunk_is_semicolon(pc) &&
          (pc->level == start->level))
   {
      if ((pc->type == CT_WORD) || (pc->type == CT_FUNC_CTOR_VAR))
      {
         UINT64 flg = pc->flags;
         if ((pc->flags & PCF_IN_ENUM) == 0)
         {
            pc->flags |= flags;
         }
         flags &= ~PCF_VAR_1ST;

         LOG_FMT(LVARDEF, "%s:%d marked '%s'[%s] in col %d flags: %#" PRIx64 " -> %#" PRIx64 "\n",
                 __func__, pc->orig_line, pc->str.c_str(),
                 get_token_name(pc->type), pc->orig_col, flg, pc->flags);
      }
      else if (chunk_is_star(pc) || chunk_is_msref(pc))
      {
         set_chunk_type(pc, CT_PTR_TYPE);
      }
      else if (chunk_is_addr(pc))
      {
         set_chunk_type(pc, CT_BYREF);
      }
      else if ((pc->type == CT_SQUARE_OPEN) || (pc->type == CT_ASSIGN))
      {
         pc = skip_expression(pc);
         continue;
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


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
 * @param start the first chunk to look at
 * @param end   the chunk after the last one to look at
 */
static bool can_be_full_param(chunk_t *start, chunk_t *end)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *last;
   int     word_cnt   = 0;
   int     type_count = 0;
   bool    ret;

   LOG_FMT(LFPARAM, "%s:", __func__);

   for (pc = start; pc != end; pc = chunk_get_next_ncnl(pc, CNAV_PREPROC))
   {
      LOG_FMT(LFPARAM, " [%s]", pc->str.c_str());

      if ((pc->type == CT_QUALIFIER) ||
          (pc->type == CT_STRUCT) ||
          (pc->type == CT_ENUM) ||
          (pc->type == CT_UNION) ||
          (pc->type == CT_TYPENAME))
      {
         LOG_FMT(LFPARAM, " <== %s! (yes)\n", get_token_name(pc->type));
         return(true);
      }

      if ((pc->type == CT_WORD) ||
          (pc->type == CT_TYPE))
      {
         word_cnt++;
         if (pc->type == CT_TYPE)
         {
            type_count++;
         }
      }
      else if ((pc->type == CT_MEMBER) ||
               (pc->type == CT_DC_MEMBER))
      {
         if (word_cnt > 0)
         {
            word_cnt--;
         }
      }
      else if ((pc != start) && chunk_is_ptr_operator(pc))
      {
         /* chunk is OK */
      }
      else if (pc->type == CT_ASSIGN)
      {
         /* chunk is OK (default values) */
         break;
      }
      else if (pc->type == CT_ANGLE_OPEN)
      {
         LOG_FMT(LFPARAM, " <== template\n");
         return(true);
      }
      else if (pc->type == CT_ELLIPSIS)
      {
         LOG_FMT(LFPARAM, " <== elipses\n");
         return(true);
      }
      else if ((word_cnt == 0) && (pc->type == CT_PAREN_OPEN))
      {
         /* Check for old-school func proto param '(type)' */
         chunk_t *tmp1 = chunk_skip_to_match(pc, CNAV_PREPROC);
         chunk_t *tmp2 = chunk_get_next_ncnl(tmp1, CNAV_PREPROC);

         if (chunk_is_token(tmp2, CT_COMMA) || chunk_is_paren_close(tmp2))
         {
            do
            {
               pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
               LOG_FMT(LFPARAM, " [%s]", pc->text());
            } while (pc != tmp1);

            /* reset some vars to allow [] after parens */
            word_cnt   = 1;
            type_count = 1;
         }
         else
         {
            LOG_FMT(LFPARAM, " <== [%s] not fcn type!\n", get_token_name(pc->type));
            return false;
         }
      }
      else if (((word_cnt == 1) || (word_cnt == type_count)) &&
               (pc->type == CT_PAREN_OPEN))
      {
         /* Check for func proto param 'void (*name)' or 'void (*name)(params)' */
         chunk_t *tmp1 = chunk_get_next_ncnl(pc, CNAV_PREPROC);
         chunk_t *tmp2 = chunk_get_next_ncnl(tmp1, CNAV_PREPROC);
         chunk_t *tmp3 = chunk_get_next_ncnl(tmp2, CNAV_PREPROC);

         if (!chunk_is_str(tmp3, ")", 1) ||
             !chunk_is_str(tmp1, "*", 1) ||
             (tmp2->type != CT_WORD))
         {
            LOG_FMT(LFPARAM, " <== [%s] not fcn type!\n", get_token_name(pc->type));
            return(false);
         }
         LOG_FMT(LFPARAM, " <skip fcn type>");
         tmp1 = chunk_get_next_ncnl(tmp3, CNAV_PREPROC);
         tmp2 = chunk_get_next_ncnl(tmp1, CNAV_PREPROC);
         if (chunk_is_str(tmp1, "(", 1))
         {
            tmp3 = chunk_skip_to_match(tmp1, CNAV_PREPROC);
         }
         pc = tmp3;

         /* reset some vars to allow [] after parens */
         word_cnt   = 1;
         type_count = 1;
      }
      else if (pc->type == CT_TSQUARE)
      {
         /* ignore it */
      }
      else if ((word_cnt == 1) && (pc->type == CT_SQUARE_OPEN))
      {
         /* skip over any array stuff */
         pc = chunk_skip_to_match(pc, CNAV_PREPROC);
      }
      else if ((word_cnt == 1) && (cpd.lang_flags & LANG_CPP) &&
               chunk_is_str(pc, "&&", 2))
      {
         /* ignore possible 'move' operator */
      }
      else
      {
         LOG_FMT(LFPARAM, " <== [%s] no way! tc=%d wc=%d\n",
                 get_token_name(pc->type), type_count, word_cnt);
         return(false);
      }
   }

   last = chunk_get_prev_ncnl(pc);
   if (chunk_is_ptr_operator(last))
   {
      LOG_FMT(LFPARAM, " <== [%s] sure!\n", get_token_name(pc->type));
      return(true);
   }

   ret = ((word_cnt >= 2) || ((word_cnt == 1) && (type_count == 1)));

   LOG_FMT(LFPARAM, " <== [%s] %s!\n",
           get_token_name(pc->type), ret ? "Yup" : "Unlikely");
   return(ret);
}


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
static void mark_function(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *prev;
   chunk_t *next;
   chunk_t *tmp;
   chunk_t *semi = NULL;
   chunk_t *paren_open;
   chunk_t *paren_close;
   chunk_t *pc_op = NULL;

   prev = chunk_get_prev_ncnlnp(pc);
   next = chunk_get_next_ncnlnp(pc);

   /* Find out what is before the operator */
   if (pc->parent_type == CT_OPERATOR)
   {
      pc_op = chunk_get_prev_type(pc, CT_OPERATOR, pc->level);
      if ((pc_op != NULL) && (pc_op->flags & PCF_EXPR_START))
      {
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      if (cpd.lang_flags & LANG_CPP)
      {
         tmp = pc;
         while ((tmp = chunk_get_prev_ncnl(tmp)) != NULL)
         {
            if ((tmp->type == CT_BRACE_CLOSE) ||
                (tmp->type == CT_SEMICOLON))
            {
               break;
            }
            if (chunk_is_paren_open(tmp))
            {
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }
            if (tmp->type == CT_ASSIGN)
            {
               set_chunk_type(pc, CT_FUNC_CALL);
               break;
            }
            if (tmp->type == CT_TEMPLATE)
            {
               set_chunk_type(pc, CT_FUNC_DEF);
               break;
            }
            if (tmp->type == CT_BRACE_OPEN)
            {
               if (tmp->parent_type == CT_FUNC_DEF)
               {
                  set_chunk_type(pc, CT_FUNC_CALL);
               }
               if ((tmp->parent_type == CT_CLASS) ||
                   (tmp->parent_type == CT_STRUCT))
               {
                  set_chunk_type(pc, CT_FUNC_DEF);
               }
               break;
            }
         }
         if ((tmp != NULL) && (pc->type != CT_FUNC_CALL))
         {
            /* Mark the return type */
            while ((tmp = chunk_get_next_ncnl(tmp)) != pc)
            {
               make_type(tmp);
            }
         }
      }
   }

   if (chunk_is_ptr_operator(next))
   {
      next = chunk_get_next_ncnlnp(next);
   }

   LOG_FMT(LFCN, "%s: %d] %s[%s] - parent=%s level=%d/%d, next=%s[%s] - level=%d\n",
           __func__,
           pc->orig_line, pc->str.c_str(),
           get_token_name(pc->type), get_token_name(pc->parent_type),
           pc->level, pc->brace_level,
           next->str.c_str(), get_token_name(next->type), next->level);

   if (pc->flags & PCF_IN_CONST_ARGS)
   {
      set_chunk_type(pc, CT_FUNC_CTOR_VAR);
      LOG_FMT(LFCN, "  1) Marked [%s] as FUNC_CTOR_VAR on line %d col %d\n",
              pc->str.c_str(), pc->orig_line, pc->orig_col);
      next = skip_template_next(next);
      flag_parens(next, 0, CT_FPAREN_OPEN, pc->type, true);
      return;
   }

   /* Skip over any template and attribute madness */
   next = skip_template_next(next);
   next = skip_attribute_next(next);

   /* Find the open and close paren */
   paren_open  = chunk_get_next_str(pc, "(", 1, pc->level);
   paren_close = chunk_get_next_str(paren_open, ")", 1, pc->level);

   if ((paren_open == NULL) || (paren_close == NULL))
   {
      LOG_FMT(LFCN, "No parens found for [%s] on line %d col %d\n",
              pc->str.c_str(), pc->orig_line, pc->orig_col);
      return;
   }

   /**
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
   if (chunk_is_str(tmp, "(", 1))
   {
      chunk_t *tmp1, *tmp2, *tmp3;

      /* skip over any leading class/namespace in: "T(F::*A)();" */
      tmp1 = chunk_get_next_ncnl(next);
      while (tmp1)
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
         tmp2 = NULL;
      }
      else
      {
         tmp3 = chunk_get_next_ncnl(tmp2);
      }

      if (chunk_is_str(tmp3, ")", 1) &&
          (chunk_is_star(tmp1) || chunk_is_msref(tmp1) ||
           ((cpd.lang_flags & LANG_OC) && chunk_is_token(tmp1, CT_CARET)))
          &&
          ((tmp2 == NULL) || (tmp2->type == CT_WORD)))
      {
         if (tmp2)
         {
            LOG_FMT(LFCN, "%s: [%d/%d] function variable [%s], changing [%s] into a type\n",
                    __func__, pc->orig_line, pc->orig_col, tmp2->text(), pc->text());
            set_chunk_type(tmp2, CT_FUNC_VAR);
            flag_parens(paren_open, 0, CT_PAREN_OPEN, CT_FUNC_VAR, false);

            LOG_FMT(LFCN, "%s: paren open @ %d:%d\n",
                    __func__, paren_open->orig_line, paren_open->orig_col);
         }
         else
         {
            LOG_FMT(LFCN, "%s: [%d/%d] function type, changing [%s] into a type\n",
                    __func__, pc->orig_line, pc->orig_col, pc->str.c_str());
            if (tmp2)
            {
               set_chunk_type(tmp2, CT_FUNC_TYPE);
            }
            flag_parens(paren_open, 0, CT_PAREN_OPEN, CT_FUNC_TYPE, false);
         }

         set_chunk_type(pc, CT_TYPE);
         set_chunk_type(tmp1, CT_PTR_TYPE);
         pc->flags &= ~PCF_VAR_1ST_DEF;
         if (tmp2 != NULL)
         {
            tmp2->flags |= PCF_VAR_1ST_DEF;
         }
         flag_parens(tmp, 0, CT_FPAREN_OPEN, CT_FUNC_PROTO, false);
         fix_fcn_def_params(tmp);
         return;
      }

      LOG_FMT(LFCN, "%s: chained function calls? [%d.%d] [%s]\n",
              __func__, pc->orig_line, pc->orig_col, pc->str.c_str());
   }

   /* Assume it is a function call if not already labeled */
   if (pc->type == CT_FUNCTION)
   {
      set_chunk_type(pc, (pc->parent_type == CT_OPERATOR) ? CT_FUNC_DEF : CT_FUNC_CALL);
   }

   /* Check for C++ function def */
   if ((pc->type == CT_FUNC_CLASS_DEF) ||
       ((prev != NULL) && ((prev->type == CT_DC_MEMBER) ||
                           (prev->type == CT_INV))))
   {
      chunk_t *destr = NULL;
      if (prev->type == CT_INV)
      {
         /* TODO: do we care that this is the destructor? */
         set_chunk_type(prev, CT_DESTRUCTOR);
         set_chunk_type(pc, CT_FUNC_CLASS_DEF);

         set_chunk_parent(pc, CT_DESTRUCTOR);

         destr = prev;
         prev  = chunk_get_prev_ncnlnp(prev);
      }

      if ((prev != NULL) && (prev->type == CT_DC_MEMBER))
      {
         prev = chunk_get_prev_ncnlnp(prev);
         // LOG_FMT(LSYS, "%s: prev1 = %s (%s)\n", __func__,
         //         get_token_name(prev->type), prev->str.c_str());
         prev = skip_template_prev(prev);
         prev = skip_attribute_prev(prev);
         // LOG_FMT(LSYS, "%s: prev2 = %s [%d](%s) pc = %s [%d](%s)\n", __func__,
         //         get_token_name(prev->type), prev->len, prev->str.c_str(),
         //         get_token_name(pc->type), pc->len, pc->str.c_str());
         if ((prev != NULL) && ((prev->type == CT_WORD) || (prev->type == CT_TYPE)))
         {
            if (pc->str.equals(prev->str))
            {
               set_chunk_type(pc, CT_FUNC_CLASS_DEF);
               LOG_FMT(LFCN, "%d:%d - FOUND %sSTRUCTOR for %s[%s]\n",
                       prev->orig_line, prev->orig_col,
                       (destr != NULL) ? "DE" : "CON",
                       prev->str.c_str(), get_token_name(prev->type));

               mark_cpp_constructor(pc);
               return;
            }
            else
            {
               /* Point to the item previous to the class name */
               prev = chunk_get_prev_ncnlnp(prev);
            }
         }
      }
   }

   /* Determine if this is a function call or a function def/proto
    * We check for level==1 to allow the case that a function prototype is
    * wrapped in a macro: "MACRO(void foo(void));"
    */
   if ((pc->type == CT_FUNC_CALL) &&
       ((pc->level == pc->brace_level) || (pc->level == 1)) &&
       ((pc->flags & PCF_IN_ARRAY_ASSIGN) == 0))
   {
      bool isa_def  = false;
      bool hit_star = false;
      LOG_FMT(LFCN, "  Checking func call: prev=%s", (prev == NULL) ? "<null>" : get_token_name(prev->type));

      // if (!chunk_ends_type(prev))
      // {
      //    goto bad_ret_type;
      // }

      /**
       * REVISIT:
       * a function def can only occur at brace level, but not inside an
       * assignment, structure, enum, or union.
       * The close paren must be followed by an open brace, with an optional
       * qualifier (const) in between.
       * There can be all sorts of template crap and/or '[]' in the type.
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
      while (prev != NULL)
      {
         if (prev->flags & PCF_IN_PREPROC)
         {
            prev = chunk_get_prev_ncnlnp(prev);
            continue;
         }

         /* Some code slips an attribute between the type and function */
         if ((prev->type == CT_FPAREN_CLOSE) &&
             (prev->parent_type == CT_ATTRIBUTE))
         {
            prev = skip_attribute_prev(prev);
            continue;
         }

         /* skip const(TYPE) */
         if ((prev->type == CT_PAREN_CLOSE) &&
             (prev->parent_type == CT_D_CAST))
         {
            LOG_FMT(LFCN, " --> For sure a prototype or definition\n");
            isa_def = true;
            break;
         }

         /** Skip the word/type before the '.' or '::' */
         if ((prev->type == CT_DC_MEMBER) ||
             (prev->type == CT_MEMBER))
         {
            prev = chunk_get_prev_ncnlnp(prev);
            if ((prev == NULL) ||
                ((prev->type != CT_WORD) &&
                 (prev->type != CT_TYPE) &&
                 (prev->type != CT_THIS)))
            {
               LOG_FMT(LFCN, " --? Skipped MEMBER and landed on %s\n",
                       (prev == NULL) ? "<null>" : get_token_name(prev->type));
               set_chunk_type(pc, CT_FUNC_CALL);
               isa_def = false;
               break;
            }
            LOG_FMT(LFCN, " <skip %s>", prev->str.c_str());
            prev = chunk_get_prev_ncnlnp(prev);
            continue;
         }

         /* If we are on a TYPE or WORD, then we must be on a proto or def */
         if ((prev->type == CT_TYPE) ||
             (prev->type == CT_WORD))
         {
            if (!hit_star)
            {
               LOG_FMT(LFCN, " --> For sure a prototype or definition\n");
               isa_def = true;
               break;
            }
            LOG_FMT(LFCN, " --> maybe a proto/def\n");
            isa_def = true;
         }

         if (chunk_is_ptr_operator(prev))
         {
            hit_star = true;
         }

         if ((prev->type != CT_OPERATOR) &&
             (prev->type != CT_TSQUARE) &&
             (prev->type != CT_ANGLE_CLOSE) &&
             (prev->type != CT_QUALIFIER) &&
             (prev->type != CT_TYPE) &&
             (prev->type != CT_WORD) &&
             !chunk_is_ptr_operator(prev))
         {
            LOG_FMT(LFCN, " --> Stopping on %s [%s]\n",
                    prev->str.c_str(), get_token_name(prev->type));
            /* certain tokens are unlikely to precede a proto or def */
            if ((prev->type == CT_ARITH) ||
                (prev->type == CT_ASSIGN) ||
                (prev->type == CT_COMMA) ||
                (prev->type == CT_STRING) ||
                (prev->type == CT_STRING_MULTI) ||
                (prev->type == CT_NUMBER) ||
                (prev->type == CT_NUMBER_FP))
            {
               isa_def = false;
            }
            break;
         }

         /* Skip over template and attribute stuff */
         if (prev->type == CT_ANGLE_CLOSE)
         {
            prev = skip_template_prev(prev);
         }
         else
         {
            prev = chunk_get_prev_ncnlnp(prev);
         }
      }

      //LOG_FMT(LFCN, " -- stopped on %s [%s]\n",
      //        prev->str.c_str(), get_token_name(prev->type));

      if (isa_def && (prev != NULL) &&
          ((chunk_is_paren_close(prev) && (prev->parent_type != CT_D_CAST)) ||
           (prev->type == CT_ASSIGN) ||
           (prev->type == CT_RETURN)))
      {
         LOG_FMT(LFCN, " -- overriding DEF due to %s [%s]\n",
                 prev->str.c_str(), get_token_name(prev->type));
         isa_def = false;
      }
      if (isa_def)
      {
         set_chunk_type(pc, CT_FUNC_DEF);
         LOG_FMT(LFCN, "%s: '%s' is FCN_DEF:", __func__, pc->str.c_str());
         if (prev == NULL)
         {
            prev = chunk_get_head();
         }
         for (tmp = prev; tmp != pc; tmp = chunk_get_next_ncnl(tmp))
         {
            LOG_FMT(LFCN, " %s[%s]",
                    tmp->str.c_str(), get_token_name(tmp->type));
            make_type(tmp);
         }
         LOG_FMT(LFCN, "\n");
      }
   }

   if (pc->type != CT_FUNC_DEF)
   {
      LOG_FMT(LFCN, "  Detected %s '%s' on line %d col %d\n",
              get_token_name(pc->type),
              pc->str.c_str(), pc->orig_line, pc->orig_col);

      tmp = flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
      if (tmp && (tmp->type == CT_BRACE_OPEN) && (tmp->parent_type != CT_DOUBLE_BRACE))
      {
         set_paren_parent(tmp, pc->type);
      }
      return;
   }

   /* We have a function definition or prototype
    * Look for a semicolon or a brace open after the close paren to figure
    * out whether this is a prototype or definition
    */

   /* See if this is a prototype or implementation */

   /* FIXME: this doesn't take the old K&R parameter definitions into account */

   /* Scan tokens until we hit a brace open (def) or semicolon (proto) */
   tmp = paren_close;
   while ((tmp = chunk_get_next_ncnl(tmp)) != NULL)
   {
      /* Only care about brace or semi on the same level */
      if (tmp->level < pc->level)
      {
         /* No semicolon - guess that it is a prototype */
         set_chunk_type(pc, CT_FUNC_PROTO);
         break;
      }
      else if (tmp->level == pc->level)
      {
         if (tmp->type == CT_BRACE_OPEN)
         {
            /* its a function def for sure */
            break;
         }
         else if (chunk_is_semicolon(tmp))
         {
            /* Set the parent for the semi for later */
            semi = tmp;
            set_chunk_type(pc, CT_FUNC_PROTO);
            break;
         }
         else if (pc->type == CT_COMMA)
         {
            set_chunk_type(pc, CT_FUNC_CTOR_VAR);
            LOG_FMT(LFCN, "  2) Marked [%s] as FUNC_CTOR_VAR on line %d col %d\n",
                    pc->str.c_str(), pc->orig_line, pc->orig_col);
            break;
         }
      }
   }

   /**
    * C++ syntax is wacky. We need to check to see if a prototype is really a
    * variable definition with parameters passed into the constructor.
    * Unfortunately, the only mostly reliable way to do so is to guess that
    * it is a constructor variable if inside a function body and scan the
    * 'parameter list' for items that are not allowed in a prototype.
    * We search backwards and checking the parent of the containing open braces.
    * If the parent is a class or namespace, then it probably is a prototype.
    */
   if ((cpd.lang_flags & LANG_CPP) &&
       (pc->type == CT_FUNC_PROTO) &&
       (pc->parent_type != CT_OPERATOR))
   {
      LOG_FMT(LFPARAM, "%s :: checking '%s' for constructor variable %s %s\n",
              __func__, pc->str.c_str(),
              get_token_name(paren_open->type),
              get_token_name(paren_close->type));

      /* Scan the parameters looking for:
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
         if ((tmp->type == CT_COMMA) && (tmp->level == (paren_open->level + 1)))
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
      if (is_param && (ref != tmp))
      {
         if (!can_be_full_param(ref, tmp))
         {
            is_param = false;
         }
      }
      if (!is_param)
      {
         set_chunk_type(pc, CT_FUNC_CTOR_VAR);
         LOG_FMT(LFCN, "  3) Marked [%s] as FUNC_CTOR_VAR on line %d col %d\n",
                 pc->str.c_str(), pc->orig_line, pc->orig_col);
      }
      else if (pc->brace_level > 0)
      {
         chunk_t *br_open = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->brace_level - 1);

         if ((br_open != NULL) &&
             (br_open->parent_type != CT_EXTERN) &&
             (br_open->parent_type != CT_NAMESPACE))
         {
            /* Do a check to see if the level is right */
            prev = chunk_get_prev_ncnl(pc);
            if (!chunk_is_str(prev, "*", 1) && !chunk_is_str(prev, "&", 1))
            {
               chunk_t *p_op = chunk_get_prev_type(pc, CT_BRACE_OPEN, pc->brace_level - 1);
               if ((p_op != NULL) &&
                   (p_op->parent_type != CT_CLASS) &&
                   (p_op->parent_type != CT_STRUCT) &&
                   (p_op->parent_type != CT_NAMESPACE))
               {
                  set_chunk_type(pc, CT_FUNC_CTOR_VAR);
                  LOG_FMT(LFCN, "  4) Marked [%s] as FUNC_CTOR_VAR on line %d col %d\n",
                          pc->str.c_str(), pc->orig_line, pc->orig_col);
               }
            }
         }
      }
   }

   if (semi != NULL)
   {
      set_chunk_parent(semi, pc->type);
   }

   flag_parens(paren_open, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, pc->type, false);

   if (pc->type == CT_FUNC_CTOR_VAR)
   {
      pc->flags |= PCF_VAR_1ST_DEF;
      return;
   }

   if (next->type == CT_TSQUARE)
   {
      next = chunk_get_next_ncnl(next);
   }

   /* Mark parameters and return type */
   fix_fcn_def_params(next);
   mark_function_return_type(pc, chunk_get_prev_ncnl(pc), pc->type);

   /* Find the brace pair and set the parent */
   if (pc->type == CT_FUNC_DEF)
   {
      tmp = chunk_get_next_ncnl(paren_close);
      while ((tmp != NULL) &&
             (tmp->type != CT_BRACE_OPEN))
      {
         //LOG_FMT(LSYS, "%s: set parent to FUNC_DEF on line %d: [%s]\n", __func__, tmp->orig_line, tmp->str.c_str());
         set_chunk_parent(tmp, CT_FUNC_DEF);
         if (!chunk_is_semicolon(tmp))
         {
            tmp->flags |= PCF_OLD_FCN_PARAMS;
         }
         tmp = chunk_get_next_ncnl(tmp);
      }
      if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
      {
         set_chunk_parent(tmp, CT_FUNC_DEF);
         tmp = chunk_skip_to_match(tmp);
         if (tmp != NULL)
         {
            set_chunk_parent(tmp, CT_FUNC_DEF);
         }
      }
   }
}


static void mark_cpp_constructor(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *paren_open;
   chunk_t *tmp;
   chunk_t *after;
   chunk_t *var;
   bool    is_destr = false;

   tmp = chunk_get_prev_ncnl(pc);
   if ((tmp->type == CT_INV) || (tmp->type == CT_DESTRUCTOR))
   {
      set_chunk_type(tmp, CT_DESTRUCTOR);
      set_chunk_parent(pc, CT_DESTRUCTOR);
      is_destr = true;
   }

   LOG_FMT(LFTOR, "%d:%d FOUND %sSTRUCTOR for %s[%s] prev=%s[%s]",
           pc->orig_line, pc->orig_col,
           is_destr ? "DE" : "CON",
           pc->str.c_str(), get_token_name(pc->type),
           tmp->str.c_str(), get_token_name(tmp->type));

   paren_open = skip_template_next(chunk_get_next_ncnl(pc));
   if (!chunk_is_str(paren_open, "(", 1))
   {
      LOG_FMT(LWARN, "%s:%d Expected '(', got: [%s]\n",
              cpd.filename, paren_open->orig_line,
              paren_open->str.c_str());
      return;
   }

   /* Mark parameters */
   fix_fcn_def_params(paren_open);
   after = flag_parens(paren_open, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CLASS_PROTO, false);

   LOG_FMT(LFTOR, "[%s]\n", after->str.c_str());

   /* Scan until the brace open, mark everything */
   tmp = paren_open;
   bool hit_colon = false;
   while ((tmp != NULL) && (tmp->type != CT_BRACE_OPEN) &&
          !chunk_is_semicolon(tmp))
   {
      tmp->flags |= PCF_IN_CONST_ARGS;
      tmp         = chunk_get_next_ncnl(tmp);
      if (chunk_is_str(tmp, ":", 1) && (tmp->level == paren_open->level))
      {
         set_chunk_type(tmp, CT_CONSTR_COLON);
         hit_colon = true;
      }
      if (hit_colon &&
          (chunk_is_paren_open(tmp) ||
           chunk_is_opening_brace(tmp)) &&
          (tmp->level == paren_open->level))
      {
         var = skip_template_prev(chunk_get_prev_ncnl(tmp));
         if ((var->type == CT_TYPE) || (var->type == CT_WORD))
         {
            set_chunk_type(var, CT_FUNC_CTOR_VAR);
            flag_parens(tmp, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CTOR_VAR, false);
         }
      }
   }
   if (tmp != NULL)
   {
      if (tmp->type == CT_BRACE_OPEN)
      {
         set_paren_parent(paren_open, CT_FUNC_CLASS_DEF);
         set_paren_parent(tmp, CT_FUNC_CLASS_DEF);
      }
      else
      {
         set_chunk_parent(tmp, CT_FUNC_CLASS_PROTO);
         set_chunk_type(pc, CT_FUNC_CLASS_PROTO);
      }
   }
}


/**
 * We're on a 'class' or 'struct'.
 * Scan for CT_FUNCTION with a string that matches pclass->str
 */
static void mark_class_ctor(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t    *next;
   chunk_t    *pclass;
   ChunkStack cs;

   pclass = chunk_get_next_ncnl(start, CNAV_PREPROC);
   if ((pclass == NULL) ||
       ((pclass->type != CT_TYPE) &&
        (pclass->type != CT_WORD)))
   {
      return;
   }

   next = chunk_get_next_ncnl(pclass, CNAV_PREPROC);
   while ((next != NULL) &&
          ((next->type == CT_TYPE) ||
           (next->type == CT_WORD) ||
           (next->type == CT_DC_MEMBER)))
   {
      pclass = next;
      next   = chunk_get_next_ncnl(next, CNAV_PREPROC);
   }

   chunk_t *pc   = chunk_get_next_ncnl(pclass, CNAV_PREPROC);
   int     level = pclass->brace_level + 1;

   if (pc == NULL)
   {
      LOG_FMT(LFTOR, "%s: Called on %s on line %d. Bailed on NULL\n",
              __func__, pclass->str.c_str(), pclass->orig_line);
      return;
   }

   /* Add the class name */
   cs.Push_Back(pclass);

   LOG_FMT(LFTOR, "%s: Called on %s on line %d (next='%s')\n",
           __func__, pclass->str.c_str(), pclass->orig_line, pc->str.c_str());

   /* detect D template class: "class foo(x) { ... }" */
   if ((cpd.lang_flags & LANG_D) && (next->type == CT_PAREN_OPEN))
   {
      set_chunk_parent(next, CT_TEMPLATE);

      next = get_d_template_types(cs, next);
      if (next && (next->type == CT_PAREN_CLOSE))
      {
         set_chunk_parent(next, CT_TEMPLATE);
      }
   }

   /* Find the open brace, abort on semicolon */
   int flags = 0;
   while ((pc != NULL) && (pc->type != CT_BRACE_OPEN))
   {
      LOG_FMT(LFTOR, " [%s]", pc->str.c_str());

      if (chunk_is_str(pc, ":", 1))
      {
         set_chunk_type(pc, CT_CLASS_COLON);
         flags |= PCF_IN_CLASS_BASE;
         LOG_FMT(LFTOR, "%s: class colon on line %d\n",
                 __func__, pc->orig_line);
      }

      if (chunk_is_semicolon(pc))
      {
         LOG_FMT(LFTOR, "%s: bailed on semicolon on line %d\n",
                 __func__, pc->orig_line);
         return;
      }
      pc->flags |= flags;
      pc         = chunk_get_next_ncnl(pc, CNAV_PREPROC);
   }

   if (pc == NULL)
   {
      LOG_FMT(LFTOR, "%s: bailed on NULL\n", __func__);
      return;
   }

   set_paren_parent(pc, start->type);

   pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
   while (pc != NULL)
   {
      pc->flags |= PCF_IN_CLASS;

      if ((pc->brace_level > level) || ((pc->flags & PCF_IN_PREPROC) != 0))
      {
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if ((pc->type == CT_BRACE_CLOSE) && (pc->brace_level < level))
      {
         LOG_FMT(LFTOR, "%s: %d] Hit brace close\n", __func__, pc->orig_line);
         pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
         if (pc && (pc->type == CT_SEMICOLON))
         {
            set_chunk_parent(pc, start->type);
         }
         return;
      }

      next = chunk_get_next_ncnl(pc, CNAV_PREPROC);
      if (chunkstack_match(cs, pc))
      {
         if ((next != NULL) && (next->len() == 1) && (next->str[0] == '('))
         {
            set_chunk_type(pc, CT_FUNC_CLASS_DEF);
            LOG_FMT(LFTOR, "%d] Marked CTor/DTor %s\n", pc->orig_line, pc->str.c_str());
            mark_cpp_constructor(pc);
         }
         else
         {
            make_type(pc);
         }
      }
      pc = next;
   }
}


/**
 * We're on a 'namespace' skip the word and then set the parent of the braces.
 */
static void mark_namespace(chunk_t *pns)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *br_close;
   bool    is_using = false;

   pc = chunk_get_prev_ncnl(pns);
   if (chunk_is_token(pc, CT_USING))
   {
      is_using = true;
      set_chunk_parent(pns, CT_USING);
   }

   pc = chunk_get_next_ncnl(pns);
   while (pc != NULL)
   {
      set_chunk_parent(pc, CT_NAMESPACE);
      if (pc->type != CT_BRACE_OPEN)
      {
         if (pc->type == CT_SEMICOLON)
         {
            if (is_using)
            {
               set_chunk_parent(pc, CT_USING);
            }
            return;
         }
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if ((cpd.settings[UO_indent_namespace_limit].n > 0) &&
          ((br_close = chunk_skip_to_match(pc)) != NULL))
      {
         int diff = br_close->orig_line - pc->orig_line;

         if (diff > cpd.settings[UO_indent_namespace_limit].n)
         {
            pc->flags       |= PCF_LONG_BLOCK;
            br_close->flags |= PCF_LONG_BLOCK;
         }
      }
      flag_parens(pc, PCF_IN_NAMESPACE, CT_NONE, CT_NAMESPACE, false);
      return;
   }
}


/**
 * Skips the D 'align()' statement and the colon, if present.
 *    align(2) int foo;  -- returns 'int'
 *    align(4):          -- returns 'int'
 *    int bar;
 */
static chunk_t *skip_align(chunk_t *start)
{
   chunk_t *pc = start;

   if (pc->type == CT_ALIGN)
   {
      pc = chunk_get_next_ncnl(pc);
      if (pc->type == CT_PAREN_OPEN)
      {
         pc = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
         pc = chunk_get_next_ncnl(pc);
         if (pc->type == CT_COLON)
         {
            pc = chunk_get_next_ncnl(pc);
         }
      }
   }
   return(pc);
}


/**
 * Examines the stuff between braces { }.
 * There should only be variable definitions and methods.
 * Skip the methods, as they will get handled elsewhere.
 */
static void mark_struct_union_body(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = start;

   while ((pc != NULL) &&
          (pc->level >= start->level) &&
          !((pc->level == start->level) && (pc->type == CT_BRACE_CLOSE)))
   {
      // LOG_FMT(LSYS, "%s: %d:%d %s:%s\n", __func__, pc->orig_line, pc->orig_col,
      //         pc->str.c_str(), get_token_name(pc->parent_type));
      if ((pc->type == CT_BRACE_OPEN) ||
          (pc->type == CT_BRACE_CLOSE) ||
          (pc->type == CT_SEMICOLON))
      {
         pc = chunk_get_next_ncnl(pc);
      }
      if (pc->type == CT_ALIGN)
      {
         pc = skip_align(pc); // "align(x)" or "align(x):"
      }
      else
      {
         pc = fix_var_def(pc);
      }
   }
}


/**
 * Sets the parent for comments.
 */
void mark_comments(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *cur;
   chunk_t *next;
   bool    prev_nl = true;
   bool    next_nl;

   cur = chunk_get_head();

   while (cur != NULL)
   {
      next    = chunk_get_next_nvb(cur);
      next_nl = (next == NULL) || chunk_is_newline(next);

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


/**
 * Marks statement starts in a macro body.
 * REVISIT: this may already be done
 */
static void mark_define_expressions(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *prev;
   bool    in_define = false;
   bool    first     = true;

   pc   = chunk_get_head();
   prev = pc;

   while (pc != NULL)
   {
      if (!in_define)
      {
         if ((pc->type == CT_PP_DEFINE) ||
             (pc->type == CT_PP_IF) ||
             (pc->type == CT_PP_ELSE))
         {
            in_define = true;
            first     = true;
         }
      }
      else
      {
         if (((pc->flags & PCF_IN_PREPROC) == 0) || (pc->type == CT_PREPROC))
         {
            in_define = false;
         }
         else
         {
            if ((pc->type != CT_MACRO) &&
                (first ||
                 (prev->type == CT_PAREN_OPEN) ||
                 (prev->type == CT_ARITH) ||
                 (prev->type == CT_CARET) ||
                 (prev->type == CT_ASSIGN) ||
                 (prev->type == CT_COMPARE) ||
                 (prev->type == CT_RETURN) ||
                 (prev->type == CT_GOTO) ||
                 (prev->type == CT_CONTINUE) ||
                 (prev->type == CT_PAREN_OPEN) ||
                 (prev->type == CT_FPAREN_OPEN) ||
                 (prev->type == CT_SPAREN_OPEN) ||
                 (prev->type == CT_BRACE_OPEN) ||
                 chunk_is_semicolon(prev) ||
                 (prev->type == CT_COMMA) ||
                 (prev->type == CT_COLON) ||
                 (prev->type == CT_QUESTION)))
            {
               pc->flags |= PCF_EXPR_START;
               first      = false;
            }
         }
      }

      prev = pc;
      pc   = chunk_get_next(pc);
   }
}


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
static void handle_cpp_template(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;
   int     level;

   tmp = chunk_get_next_ncnl(pc);
   if (tmp->type != CT_ANGLE_OPEN)
   {
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);

   level = tmp->level;

   while ((tmp = chunk_get_next(tmp)) != NULL)
   {
      if ((tmp->type == CT_CLASS) ||
          (tmp->type == CT_STRUCT))
      {
         set_chunk_type(tmp, CT_TYPE);
      }
      else if ((tmp->type == CT_ANGLE_CLOSE) && (tmp->level == level))
      {
         set_chunk_parent(tmp, CT_TEMPLATE);
         break;
      }
   }
   if (tmp != NULL)
   {
      tmp = chunk_get_next_ncnl(tmp);
      if ((tmp != NULL) &&
          ((tmp->type == CT_CLASS) || (tmp->type == CT_STRUCT)))
      {
         set_chunk_parent(tmp, CT_TEMPLATE);

         /* REVISTI: This may be a bit risky - might need to track the { }; */
         tmp = chunk_get_next_type(tmp, CT_SEMICOLON, tmp->level);
         if (tmp != NULL)
         {
            set_chunk_parent(tmp, CT_TEMPLATE);
         }
      }
   }
}


/**
 * Verify and then mark C++ lambda expressions.
 * The expected format is '[...](...){...}' or '[...](...) -> type {...}'
 * sq_o is '[' CT_SQUARE_OPEN or '[]' CT_TSQUARE
 * Split the '[]' so we can control the space
 */
static void handle_cpp_lambda(chunk_t *sq_o)
{
   LOG_FUNC_ENTRY();
   chunk_t *sq_c;
   chunk_t *pa_o;
   chunk_t *pa_c;
   chunk_t *br_o;
   chunk_t *br_c;
   chunk_t *ret = NULL;

   sq_c = sq_o; /* assuming '[]' */
   if (sq_o->type == CT_SQUARE_OPEN)
   {
      /* make sure there is a ']' */
      sq_c = chunk_skip_to_match(sq_o);
      if (!sq_c)
      {
         return;
      }
   }

   /* Make sure a '(' is next */
   pa_o = chunk_get_next_ncnl(sq_c);
   if (!pa_o || (pa_o->type != CT_PAREN_OPEN))
   {
      return;
   }
   /* and now find the ')' */
   pa_c = chunk_skip_to_match(pa_o);
   if (!pa_c)
   {
      return;
   }

   /* Check if keyword 'mutable' is before '->' */
   br_o = chunk_get_next_ncnl(pa_c);
   if (chunk_is_str(br_o, "mutable", 7))
   {
      br_o = chunk_get_next_ncnl(br_o);
   }

   /* Make sure a '{' or '->' is next */
   if (chunk_is_str(br_o, "->", 2))
   {
      ret = br_o;
      /* REVISIT: really should check the stuff we are skipping */
      br_o = chunk_get_next_type(br_o, CT_BRACE_OPEN, br_o->level);
   }
   if (!br_o || (br_o->type != CT_BRACE_OPEN))
   {
      return;
   }
   /* and now find the '}' */
   br_c = chunk_skip_to_match(br_o);
   if (!br_c)
   {
      return;
   }

   /* This looks like a lambda expression */
   if (sq_o->type == CT_TSQUARE)
   {
      /* split into two chunks */
      chunk_t nc;

      nc = *sq_o;
      set_chunk_type(sq_o, CT_SQUARE_OPEN);
      sq_o->str.resize(1);
      sq_o->orig_col_end = sq_o->orig_col + 1;

      nc.type = CT_SQUARE_CLOSE;
      nc.str.pop_front();
      nc.orig_col++;
      nc.column++;
      sq_c = chunk_add_after(&nc, sq_o);
   }
   set_chunk_parent(sq_o, CT_CPP_LAMBDA);
   set_chunk_parent(sq_c, CT_CPP_LAMBDA);
   set_chunk_type(pa_o, CT_FPAREN_OPEN);
   set_chunk_parent(pa_o, CT_CPP_LAMBDA);
   set_chunk_type(pa_c, CT_FPAREN_CLOSE);
   set_chunk_parent(pa_c, CT_CPP_LAMBDA);
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

   fix_fcn_def_params(pa_o);
}


/**
 * Parse off the types in the D template args, adds to cs
 * returns the close_paren
 */
static chunk_t *get_d_template_types(ChunkStack& cs, chunk_t *open_paren)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp       = open_paren;
   bool    maybe_type = true;

   while (((tmp = chunk_get_next_ncnl(tmp)) != NULL) &&
          (tmp->level > open_paren->level))
   {
      if ((tmp->type == CT_TYPE) || (tmp->type == CT_WORD))
      {
         if (maybe_type)
         {
            make_type(tmp);
            cs.Push_Back(tmp);
         }
         maybe_type = false;
      }
      else if (tmp->type == CT_COMMA)
      {
         maybe_type = true;
      }
   }
   return tmp;
}


static bool chunkstack_match(ChunkStack& cs, chunk_t *pc)
{
   chunk_t *tmp;
   int     idx;

   for (idx = 0; idx < cs.Len(); idx++)
   {
      tmp = cs.GetChunk(idx);
      if (pc->str.equals(tmp->str))
      {
         return true;
      }
   }
   return false;
}


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
static void handle_d_template(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *name;
   chunk_t *po;
   chunk_t *tmp;

   name = chunk_get_next_ncnl(pc);
   po   = chunk_get_next_ncnl(name);
   if (!name || ((name->type != CT_WORD) && (name->type != CT_WORD)))
   {
      /* TODO: log an error, expected NAME */
      return;
   }
   if (!po || (po->type != CT_PAREN_OPEN))
   {
      /* TODO: log an error, expected '(' */
      return;
   }

   set_chunk_type(name, CT_TYPE);
   set_chunk_parent(name, CT_TEMPLATE);
   set_chunk_parent(po, CT_TEMPLATE);

   ChunkStack cs;

   tmp = get_d_template_types(cs, po);

   if (!tmp || (tmp->type != CT_PAREN_CLOSE))
   {
      /* TODO: log an error, expected ')' */
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);

   tmp = chunk_get_next_ncnl(tmp);
   if (tmp->type != CT_BRACE_OPEN)
   {
      /* TODO: log an error, expected '{' */
      return;
   }
   set_chunk_parent(tmp, CT_TEMPLATE);
   po = tmp;

   tmp = po;
   while (((tmp = chunk_get_next_ncnl(tmp)) != NULL) &&
          (tmp->level > po->level))
   {
      if ((tmp->type == CT_WORD) && chunkstack_match(cs, tmp))
      {
         set_chunk_type(tmp, CT_TYPE);
      }
   }
   if (tmp->type != CT_BRACE_CLOSE)
   {
      /* TODO: log an error, expected '}' */
   }
   set_chunk_parent(tmp, CT_TEMPLATE);
}


/**
 * We are on a word followed by a angle open which is part of a template.
 * If the angle close is followed by a open paren, then we are on a template
 * function def or a template function call:
 *   Vector2<float>(...) [: ...[, ...]] { ... }
 * Or we could be on a variable def if it's followed by a word:
 *   Renderer<rgb32> rend;
 */
static void mark_template_func(chunk_t *pc, chunk_t *pc_next)
{
   LOG_FUNC_ENTRY();
   chunk_t *angle_close;
   chunk_t *after;

   /* We know angle_close must be there... */
   angle_close = chunk_get_next_type(pc_next, CT_ANGLE_CLOSE, pc->level);

   after = chunk_get_next_ncnl(angle_close);
   if (after != NULL)
   {
      if (chunk_is_str(after, "(", 1))
      {
         if (angle_close->flags & PCF_IN_FCN_CALL)
         {
            LOG_FMT(LTEMPFUNC, "%s: marking '%s' in line %d as a FUNC_CALL\n",
                    __func__, pc->str.c_str(), pc->orig_line);
            set_chunk_type(pc, CT_FUNC_CALL);
            flag_parens(after, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_FUNC_CALL, false);
         }
         else
         {
            /* Might be a function def. Must check what is before the template:
             * Func call:
             *   BTree.Insert(std::pair<int, double>(*it, double(*it) + 1.0));
             *   a = Test<int>(j);
             *   std::pair<int, double>(*it, double(*it) + 1.0));
             */

            LOG_FMT(LTEMPFUNC, "%s: marking '%s' in line %d as a FUNC_CALL 2\n",
                    __func__, pc->str.c_str(), pc->orig_line);
            // its a function!!!
            set_chunk_type(pc, CT_FUNC_CALL);
            mark_function(pc);
         }
      }
      else if (after->type == CT_WORD)
      {
         // its a type!
         set_chunk_type(pc, CT_TYPE);
         pc->flags    |= PCF_VAR_TYPE;
         after->flags |= PCF_VAR_DEF;
      }
   }
}


/**
 * Just mark every CT_WORD until a semicolon as CT_SQL_WORD.
 * Adjust the levels if pc is CT_SQL_BEGIN
 */
static void mark_exec_sql(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;

   /* Change CT_WORD to CT_SQL_WORD */
   for (tmp = chunk_get_next(pc); tmp != NULL; tmp = chunk_get_next(tmp))
   {
      set_chunk_parent(tmp, pc->type);
      if (tmp->type == CT_WORD)
      {
         set_chunk_type(tmp, CT_SQL_WORD);
      }
      if (tmp->type == CT_SEMICOLON)
      {
         break;
      }
   }

   if ((pc->type != CT_SQL_BEGIN) ||
       (tmp == NULL) || (tmp->type != CT_SEMICOLON))
   {
      return;
   }

   for (tmp = chunk_get_next(tmp);
        (tmp != NULL) && (tmp->type != CT_SQL_END);
        tmp = chunk_get_next(tmp))
   {
      tmp->level++;
   }
}


/**
 * Skips over the rest of the template if ang_open is indeed a CT_ANGLE_OPEN.
 * Points to the chunk after the CT_ANGLE_CLOSE.
 * If the chunk isn't an CT_ANGLE_OPEN, then it is returned.
 */
chunk_t *skip_template_next(chunk_t *ang_open)
{
   if ((ang_open != NULL) && (ang_open->type == CT_ANGLE_OPEN))
   {
      chunk_t *pc;
      pc = chunk_get_next_type(ang_open, CT_ANGLE_CLOSE, ang_open->level);
      return(chunk_get_next_ncnl(pc));
   }
   return(ang_open);
}


/**
 * Skips over the rest of the template if ang_close is indeed a CT_ANGLE_CLOSE.
 * Points to the chunk before the CT_ANGLE_OPEN
 * If the chunk isn't an CT_ANGLE_CLOSE, then it is returned.
 */
chunk_t *skip_template_prev(chunk_t *ang_close)
{
   if ((ang_close != NULL) && (ang_close->type == CT_ANGLE_CLOSE))
   {
      chunk_t *pc;
      pc = chunk_get_prev_type(ang_close, CT_ANGLE_OPEN, ang_close->level);
      return(chunk_get_prev_ncnl(pc));
   }
   return(ang_close);
}


/**
 * Skips the rest of the array definitions if ary_def is indeed a
 * CT_TSQUARE or CT_SQUARE_OPEN
 */
chunk_t *skip_tsquare_next(chunk_t *ary_def)
{
   if (ary_def && ((ary_def->type == CT_SQUARE_OPEN) ||
                   (ary_def->type == CT_TSQUARE)))
   {
      return chunk_get_next_nisq(ary_def);
   }
   return(ary_def);
}


/**
 * If attr is CT_ATTRIBUTE, then skip it and the parens and return the chunk
 * after the CT_FPAREN_CLOSE.
 * If the chunk isn't an CT_ATTRIBUTE, then it is returned.
 */
chunk_t *skip_attribute_next(chunk_t *attr)
{
   if ((attr != NULL) && (attr->type == CT_ATTRIBUTE))
   {
      chunk_t *pc = chunk_get_next(attr);
      if ((pc != NULL) && (pc->type == CT_FPAREN_OPEN))
      {
         pc = chunk_get_next_type(attr, CT_FPAREN_CLOSE, attr->level);
         return(chunk_get_next_ncnl(pc));
      }
      return(pc);
   }
   return(attr);
}


/**
 * If fp_close is a CT_FPAREN_CLOSE with a parent of CT_ATTRIBUTE, then skip it
 * and the '__attribute__' thingy and return the chunk before CT_ATTRIBUTE.
 * Otherwise return fp_close.
 */
chunk_t *skip_attribute_prev(chunk_t *fp_close)
{
   if ((fp_close != NULL) &&
       (fp_close->type == CT_FPAREN_CLOSE) &&
       (fp_close->parent_type == CT_ATTRIBUTE))
   {
      chunk_t *pc;
      pc = chunk_get_prev_type(fp_close, CT_ATTRIBUTE, fp_close->level);
      return(chunk_get_prev_ncnl(pc));
   }
   return(fp_close);
}


/**
 * Process an ObjC 'class'
 * pc is the chunk after '@implementation' or '@interface' or '@protocol'.
 * Change colons, etc. Processes stuff until '@end'.
 * Skips anything in braces.
 */
static void handle_oc_class(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;
   bool    hit_scope = false;
   int     do_pl     = 1;

   LOG_FMT(LOCCLASS, "%s: start [%s] [%s] line %d\n", __func__,
           pc->str.c_str(), get_token_name(pc->parent_type), pc->orig_line);

   if (pc->parent_type == CT_OC_PROTOCOL)
   {
      tmp = chunk_get_next_ncnl(pc);
      if (chunk_is_semicolon(tmp))
      {
         set_chunk_parent(tmp, pc->parent_type);
         LOG_FMT(LOCCLASS, "%s:   bail on semicolon\n", __func__);
         return;
      }
   }

   tmp = pc;
   while ((tmp = chunk_get_next_nnl(tmp)) != NULL)
   {
      LOG_FMT(LOCCLASS, "%s:       %d [%s]\n", __func__,
              tmp->orig_line, tmp->str.c_str());

      if (tmp->type == CT_OC_END)
      {
         break;
      }
      if ((do_pl == 1) && chunk_is_str(tmp, "<", 1))
      {
         set_chunk_type(tmp, CT_ANGLE_OPEN);
         set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         do_pl = 2;
      }
      if ((do_pl == 2) && chunk_is_str(tmp, ">", 1))
      {
         set_chunk_type(tmp, CT_ANGLE_CLOSE);
         set_chunk_parent(tmp, CT_OC_PROTO_LIST);
         do_pl = 0;
      }
      if (tmp->type == CT_BRACE_OPEN)
      {
         do_pl = 0;
         set_chunk_parent(tmp, CT_OC_CLASS);
         tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);
         if (tmp != NULL)
         {
            set_chunk_parent(tmp, CT_OC_CLASS);
         }
      }
      else if (tmp->type == CT_COLON)
      {
         set_chunk_type(tmp, hit_scope ? CT_OC_COLON : CT_CLASS_COLON);
         if (tmp->type == CT_CLASS_COLON)
         {
            set_chunk_parent(tmp, CT_OC_CLASS);
         }
      }
      else if (chunk_is_str(tmp, "-", 1) || chunk_is_str(tmp, "+", 1))
      {
         do_pl = 0;
         if (chunk_is_newline(chunk_get_prev(tmp)))
         {
            set_chunk_type(tmp, CT_OC_SCOPE);
            tmp->flags |= PCF_STMT_START;
            hit_scope   = true;
         }
      }
      if (do_pl == 2)
      {
         set_chunk_parent(tmp, CT_OC_PROTO_LIST);
      }
   }

   if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
   {
      tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);
      if (tmp != NULL)
      {
         set_chunk_parent(tmp, CT_OC_CLASS);
      }
   }
}


/* Mark Objective-C blocks (aka lambdas or closures)
 *  The syntax and usage is exactly like C function pointers
 *  but instead of an asterisk they have a caret as pointer symbol.
 *  Although it may look expensive this functions is only triggered
 *  on appearance of an OC_BLOCK_CARET for LANG_OC.
 *  repeat(10, ^{ putc('0'+d); });
 *  typedef void (^workBlk_t)(void);
 *
 * @param pc points to the '^'
 */
static void handle_oc_block_literal(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp  = pc;
   chunk_t *prev = chunk_get_prev_ncnl(pc);
   chunk_t *next = chunk_get_next_ncnl(pc);

   if (!pc || !prev || !next)
   {
      return; /* let's be paranoid */
   }

   chunk_t *apo;  /* arg paren open */
   chunk_t *apc;  /* arg paren close */
   chunk_t *bbo;  /* block brace open */
   chunk_t *bbc;  /* block brace close */

   /* block literal: '^ RTYPE ( ARGS ) { }'
    * RTYPE and ARGS are optional
    */
   LOG_FMT(LOCBLK, "%s: block literal @ %d:%d\n", __func__, pc->orig_line, pc->orig_col);

   apo = NULL;
   bbo = NULL;

   LOG_FMT(LOCBLK, "%s:  + scan", __func__);
   for (tmp = next; tmp; tmp = chunk_get_next_ncnl(tmp))
   {
      LOG_FMT(LOCBLK, " %s", tmp->text());
      if ((tmp->level < pc->level) || (tmp->type == CT_SEMICOLON))
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
         if (tmp->type == CT_BRACE_OPEN)
         {
            LOG_FMT(LOCBLK, "[BRACE]");
            bbo = tmp;
            break;
         }
      }
   }

   /* make sure we have braces */
   bbc = chunk_skip_to_match(bbo);
   if (!bbo || !bbc)
   {
      LOG_FMT(LOCBLK, " -- no braces found\n");
      return;
   }
   LOG_FMT(LOCBLK, "\n");

   /* we are on a block literal for sure */
   set_chunk_type(pc, CT_OC_BLOCK_CARET);
   set_chunk_parent(pc, CT_OC_BLOCK_EXPR);

   /* handle the optional args */
   chunk_t *lbp; /* last before paren - end of return type, if any */
   if (apo)
   {
      apc = chunk_skip_to_match(apo);
      if (chunk_is_paren_close(apc))
      {
         LOG_FMT(LOCBLK, " -- marking parens @ %d:%d and %d:%d\n",
                 apo->orig_line, apo->orig_col, apc->orig_line, apc->orig_col);
         flag_parens(apo, PCF_OC_ATYPE, CT_FPAREN_OPEN, CT_OC_BLOCK_EXPR, true);
         fix_fcn_def_params(apo);
      }
      lbp = chunk_get_prev_ncnl(apo);
   }
   else
   {
      lbp = chunk_get_prev_ncnl(bbo);
   }

   /* mark the return type, if any */
   while (lbp != pc)
   {
      LOG_FMT(LOCBLK, " -- lbp %s[%s]\n", lbp->text(), get_token_name(lbp->type));
      make_type(lbp);
      lbp->flags |= PCF_OC_RTYPE;
      set_chunk_parent(lbp, CT_OC_BLOCK_EXPR);
      lbp = chunk_get_prev_ncnl(lbp);
   }
   /* mark the braces */
   set_chunk_parent(bbo, CT_OC_BLOCK_EXPR);
   set_chunk_parent(bbc, CT_OC_BLOCK_EXPR);
}


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
 * @param pc points to the '^'
 */
static void handle_oc_block_type(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   if (!pc)
   {
      return;
   }

   if (pc->flags & PCF_IN_TYPEDEF)
   {
      LOG_FMT(LOCBLK, "%s: skip block type @ %d:%d -- in typedef\n",
              __func__, pc->orig_line, pc->orig_col);
      return;
   }

   chunk_t *tpo;  /* type paren open */
   chunk_t *tpc;  /* type paren close */
   chunk_t *nam;  /* name (if any) of '^' */
   chunk_t *apo;  /* arg paren open */
   chunk_t *apc;  /* arg paren close */

   /* make sure we have '( ^' */
   tpo = chunk_get_prev_ncnl(pc);
   if (chunk_is_paren_open(tpo))
   {
      /* block type: 'RTYPE (^LABEL)(ARGS)'
       * LABEL is optional.
       */
      tpc = chunk_skip_to_match(tpo);  /* type close paren (after '^') */
      nam = chunk_get_prev_ncnl(tpc);  /* name (if any) or '^' */
      apo = chunk_get_next_ncnl(tpc);  /* arg open paren */
      apc = chunk_skip_to_match(apo);  /* arg close paren */

      // If this is a block literal instead of a block type, 'nam' will actually
      // be the closing bracket of the block.
      // We run into this situation if a block literal is enclosed in parentheses.
      if (chunk_is_closing_brace(nam))
      {
         return handle_oc_block_literal(pc);
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
         else if (chunk_is_token(aft, CT_ASSIGN) || chunk_is_token(aft, CT_SEMICOLON))
         {
            set_chunk_type(nam, CT_FUNC_VAR);
            pt = CT_FUNC_VAR;
         }
         else
         {
            set_chunk_type(nam, CT_FUNC_TYPE);
            pt = CT_FUNC_TYPE;
         }
         LOG_FMT(LOCBLK, "%s: block type @ %d:%d (%s)[%s]\n", __func__,
                 pc->orig_line, pc->orig_col, nam->text(), get_token_name(nam->type));
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
         mark_function_return_type(nam, chunk_get_prev_ncnl(tpo), pt);
      }
   }
}


/**
 * Process a type that is enclosed in parens in message decls.
 * TODO: handle block types, which get special formatting
 *
 * @param pc points to the open paren
 * @return the chunk after the type
 */
static chunk_t *handle_oc_md_type(chunk_t *paren_open, c_token_t ptype, UINT64 flags, bool& did_it)
{
   chunk_t *paren_close;

   if (!chunk_is_paren_open(paren_open) ||
       ((paren_close = chunk_skip_to_match(paren_open)) == NULL))
   {
      did_it = false;
      return paren_open;
   }

   did_it = true;

   set_chunk_parent(paren_open, ptype);
   paren_open->flags |= flags;
   set_chunk_parent(paren_close, ptype);
   paren_close->flags |= flags;

   for (chunk_t *cur = chunk_get_next_ncnl(paren_open);
        cur != paren_close;
        cur = chunk_get_next_ncnl(cur))
   {
      LOG_FMT(LOCMSGD, " <%s|%s>", cur->text(), get_token_name(cur->type));
      cur->flags |= flags;
      make_type(cur);
   }

   /* returning the chunk after the paren close */
   return chunk_get_next_ncnl(paren_close);
}


/**
 * Process an ObjC message spec/dec
 *
 * Specs:
 * -(void) foo ARGS;
 *
 * Decl:
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
static void handle_oc_message_decl(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t   *tmp;
   bool      in_paren  = false;
   int       paren_cnt = 0;
   int       arg_cnt   = 0;
   c_token_t pt;
   bool      did_it;

   /* Figure out if this is a spec or decl */
   tmp = pc;
   while ((tmp = chunk_get_next(tmp)) != NULL)
   {
      if (tmp->level < pc->level)
      {
         /* should not happen */
         return;
      }
      if ((tmp->type == CT_SEMICOLON) ||
          (tmp->type == CT_BRACE_OPEN))
      {
         break;
      }
   }
   if (tmp == NULL)
   {
      return;
   }
   pt = (tmp->type == CT_SEMICOLON) ? CT_OC_MSG_SPEC : CT_OC_MSG_DECL;

   set_chunk_type(pc, CT_OC_SCOPE);
   set_chunk_parent(pc, pt);

   LOG_FMT(LOCMSGD, "%s: %s @ %d:%d -", __func__, get_token_name(pt), pc->orig_line, pc->orig_col);

   /* format: -(TYPE) NAME [: (TYPE)NAME */

   /* handle the return type */
   tmp = handle_oc_md_type(chunk_get_next_ncnl(pc), pt, PCF_OC_RTYPE, did_it);
   if (!did_it)
   {
      LOG_FMT(LOCMSGD, " -- missing type parens\n");
      return;
   }

   /* expect the method name/label */
   if (!chunk_is_token(tmp, CT_WORD))
   {
      LOG_FMT(LOCMSGD, " -- missing method name\n");
      return;
   }

   chunk_t *label = tmp;
   set_chunk_type(tmp, pt);
   set_chunk_parent(tmp, pt);
   pc = chunk_get_next_ncnl(tmp);

   LOG_FMT(LOCMSGD, " [%s]%s", pc->text(), get_token_name(pc->type));

   /* if we have a colon next, we have args */
   if ((pc->type == CT_COLON) || (pc->type == CT_OC_COLON))
   {
      pc = label;

      while (true)
      {
         /* skip optional label */
         if (chunk_is_token(pc, CT_WORD) || chunk_is_token(pc, pt))
         {
            set_chunk_parent(pc, pt);
            pc = chunk_get_next_ncnl(pc);
         }
         /* a colon must be next */
         if (!chunk_is_str(pc, ":", 1))
         {
            break;
         }
         set_chunk_type(pc, CT_OC_COLON);
         set_chunk_parent(pc, pt);
         pc = chunk_get_next_ncnl(pc);

         /* next is the type in parens */
         LOG_FMT(LOCMSGD, "  (%s)", pc->text());
         tmp = handle_oc_md_type(pc, pt, PCF_OC_ATYPE, did_it);
         if (!did_it)
         {
            LOG_FMT(LWARN, "%s: %d:%d expected type\n", __func__, pc->orig_line, pc->orig_col);
            break;
         }
         pc = tmp;
         /* we should now be on the arg name */
         pc->flags |= PCF_VAR_DEF;
         LOG_FMT(LOCMSGD, " arg[%s]", pc->text());
         pc = chunk_get_next_ncnl(pc);
      }
   }

   LOG_FMT(LOCMSGD, " end[%s]", pc->text());

   if (chunk_is_token(pc, CT_BRACE_OPEN))
   {
      set_chunk_parent(pc, pt);
      pc = chunk_skip_to_match(pc);
      if (pc)
      {
         set_chunk_parent(pc, pt);
      }
   }
   else if (chunk_is_token(pc, CT_SEMICOLON))
   {
      set_chunk_parent(pc, pt);
   }

   LOG_FMT(LOCMSGD, "\n");
   return;

   /* Mark everything */
   tmp = pc;
   while ((tmp = chunk_get_next(tmp)) != NULL)
   {
      LOG_FMT(LOCMSGD, " [%s]", tmp->text());

      if ((tmp->type == CT_SEMICOLON) ||
          (tmp->type == CT_BRACE_OPEN))
      {
         set_chunk_parent(tmp, pt);
         break;
      }

      /* Mark first parens as return type */
      if ((arg_cnt == 0) &&
          ((tmp->type == CT_PAREN_OPEN) ||
           (tmp->type == CT_PAREN_CLOSE)))
      {
         set_chunk_parent(tmp, CT_OC_RTYPE);
         in_paren = (tmp->type == CT_PAREN_OPEN);
         if (!in_paren)
         {
            paren_cnt++;
            arg_cnt++;
         }
      }
      else if ((tmp->type == CT_PAREN_OPEN) ||
               (tmp->type == CT_PAREN_CLOSE))
      {
         set_chunk_parent(tmp, pt);
         in_paren = (tmp->type == CT_PAREN_OPEN);
         if (!in_paren)
         {
            paren_cnt++;
         }
      }
      else if (tmp->type == CT_WORD)
      {
         if (in_paren)
         {
            set_chunk_type(tmp, CT_TYPE);
            set_chunk_parent(tmp, pt);
         }
         else if (paren_cnt == 1)
         {
            set_chunk_type(tmp, pt);
         }
         else
         {
            tmp->flags |= PCF_VAR_DEF;
         }
      }
      else if (tmp->type == CT_COLON)
      {
         set_chunk_type(tmp, CT_OC_COLON);
         set_chunk_parent(tmp, pt);
      }
   }

   if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
   {
      tmp = chunk_skip_to_match(tmp);
      if (tmp)
      {
         set_chunk_parent(tmp, pt);
      }
   }
   LOG_FMT(LOCMSGD, "\n");
}


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
 * @param os points to the open square '['
 */
static void handle_oc_message_send(chunk_t *os)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;
   chunk_t *cs = chunk_get_next(os);

   while ((cs != NULL) && (cs->level > os->level))
   {
      cs = chunk_get_next(cs);
   }

   if ((cs == NULL) || (cs->type != CT_SQUARE_CLOSE))
   {
      return;
   }

   LOG_FMT(LOCMSG, "%s: line %d, col %d\n", __func__, os->orig_line, os->orig_col);

   tmp = chunk_get_next_ncnl(cs);
   if (chunk_is_semicolon(tmp))
   {
      set_chunk_parent(tmp, CT_OC_MSG);
   }

   set_chunk_parent(os, CT_OC_MSG);
   os->flags |= PCF_IN_OC_MSG;
   set_chunk_parent(cs, CT_OC_MSG);
   cs->flags |= PCF_IN_OC_MSG;

   /* expect a word first thing or [...] */
   tmp = chunk_get_next_ncnl(os);
   if ((tmp->type == CT_SQUARE_OPEN) || (tmp->type == CT_PAREN_OPEN))
   {
      tmp = chunk_skip_to_match(tmp);
   }
   else if ((tmp->type != CT_WORD) && (tmp->type != CT_TYPE) && (tmp->type != CT_STRING))
   {
      LOG_FMT(LOCMSG, "%s: %d:%d expected identifier, not '%s' [%s]\n", __func__,
              tmp->orig_line, tmp->orig_col,
              tmp->text(), get_token_name(tmp->type));
      return;
   }
   else
   {
      chunk_t *tt = chunk_get_next_ncnl(tmp);
      if (chunk_is_paren_open(tt))
      {
         set_chunk_type(tmp, CT_FUNC_CALL);
         tmp = chunk_get_prev_ncnl(set_paren_parent(tt, CT_FUNC_CALL));
      }
      else
      {
         set_chunk_type(tmp, CT_OC_MSG_CLASS);
      }
   }

   /* handle '< protocol >' */
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

   if (tmp && ((tmp->type == CT_WORD) || (tmp->type == CT_TYPE)))
   {
      set_chunk_type(tmp, CT_OC_MSG_FUNC);
   }

   chunk_t *prev = NULL;

   for (tmp = chunk_get_next(os); tmp != cs; tmp = chunk_get_next(tmp))
   {
      tmp->flags |= PCF_IN_OC_MSG;
      if (tmp->level == cs->level + 1)
      {
         if (tmp->type == CT_COLON)
         {
            set_chunk_type(tmp, CT_OC_COLON);
            if ((prev != NULL) && ((prev->type == CT_WORD) || (prev->type == CT_TYPE)))
            {
               /* Might be a named param, check previous block */
               chunk_t *pp = chunk_get_prev(prev);
               if ((pp != NULL) &&
                   (pp->type != CT_OC_COLON) &&
                   (pp->type != CT_ARITH) &&
                   (pp->type != CT_CARET))
               {
                  set_chunk_type(prev, CT_OC_MSG_NAME);
                  set_chunk_parent(tmp, CT_OC_MSG_NAME);
               }
            }
         }
      }
      prev = tmp;
   }
}


/**
 * Process an C# [] thingy:
 *    [assembly: xxx]
 *    [AttributeUsage()]
 *    [@X]
 *
 * Set the next chunk to a statement start after the close ']'
 *
 * @param os points to the open square '['
 */
static void handle_cs_square_stmt(chunk_t *os)
{
   LOG_FUNC_ENTRY();
   chunk_t *tmp;
   chunk_t *cs = chunk_get_next(os);

   while ((cs != NULL) && (cs->level > os->level))
   {
      cs = chunk_get_next(cs);
   }

   if ((cs == NULL) || (cs->type != CT_SQUARE_CLOSE))
   {
      return;
   }

   set_chunk_parent(os, CT_CS_SQ_STMT);
   set_chunk_parent(cs, CT_CS_SQ_STMT);

   for (tmp = chunk_get_next(os); tmp != cs; tmp = chunk_get_next(tmp))
   {
      set_chunk_parent(tmp, CT_CS_SQ_STMT);
      if (tmp->type == CT_COLON)
      {
         set_chunk_type(tmp, CT_CS_SQ_COLON);
      }
   }

   tmp = chunk_get_next_ncnl(cs);
   if (tmp != NULL)
   {
      tmp->flags |= PCF_STMT_START | PCF_EXPR_START;
   }
}


/**
 * We are on a brace open that is preceded by a word or square close.
 * Set the brace parent to CT_CS_PROPERTY and find the first item in the
 * property and set its parent, too.
 */
static void handle_cs_property(chunk_t *bro)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   bool    did_prop = false;

   set_paren_parent(bro, CT_CS_PROPERTY);

   pc = bro;
   while ((pc = chunk_get_prev_ncnl(pc)) != NULL)
   {
      if (pc->level == bro->level)
      {
         if (!did_prop && ((pc->type == CT_WORD) || (pc->type == CT_THIS)))
         {
            set_chunk_type(pc, CT_CS_PROPERTY);
            did_prop = true;
         }
         else
         {
            set_chunk_parent(pc, CT_CS_PROPERTY);
            make_type(pc);
         }
         if (pc->flags & PCF_STMT_START)
         {
            break;
         }
      }
   }
}


/**
 * We hit a ']' followed by a WORD. This may be a multidimensional array type.
 * Example: int[,,] x;
 * If there is nothing but commas between the open and close, then mark it.
 */
static void handle_cs_array_type(chunk_t *pc)
{
   chunk_t *prev = chunk_get_prev(pc);

   for (prev = chunk_get_prev(pc);
        prev && (prev->type == CT_COMMA);
        prev = chunk_get_prev(prev))
   {
      /* empty */
   }

   if (prev && (prev->type == CT_SQUARE_OPEN))
   {
      while (pc != prev)
      {
         pc->parent_type = CT_TYPE;
         pc = chunk_get_prev(pc);
      }
      prev->parent_type = CT_TYPE;
   }
}


/**
 * Remove 'return;' that appears as the last statement in a function
 */
void remove_extra_returns()
{
   LOG_FUNC_ENTRY();
   chunk_t *pc;
   chunk_t *semi;
   chunk_t *cl_br;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if ((pc->type == CT_RETURN) && ((pc->flags & PCF_IN_PREPROC) == 0))
      {
         semi  = chunk_get_next_ncnl(pc);
         cl_br = chunk_get_next_ncnl(semi);

         if ((semi != NULL) && (semi->type == CT_SEMICOLON) &&
             (cl_br != NULL) && (cl_br->type == CT_BRACE_CLOSE) &&
             ((cl_br->parent_type == CT_FUNC_DEF) ||
              (cl_br->parent_type == CT_FUNC_CLASS_DEF)))
         {
            LOG_FMT(LRMRETURN, "Removed 'return;' on line %d\n", pc->orig_line);
            chunk_del(pc);
            chunk_del(semi);
            pc = cl_br;
         }
      }

      pc = chunk_get_next(pc);
   }
}

/**
 * Remove 'return;' that appears as the last statement in a function
 */
void remove_brace_only_has_return()
{
    
    LOG_FUNC_ENTRY();
    
    chunk_t *pc;
    chunk_t *prev_pc;         /*'{'*/
    chunk_t *next_pr;         /*'}'*/
    chunk_t *prev_prev_pc;   /*before '{' */
    chunk_t *next_prev_pc;   /*back '{' */
    chunk_t *prev_next_pc;  /*before '}'*/
    chunk_t *next_next_pc;  /*back '}'*/
    
    pc = chunk_get_head();
    while (pc != NULL)
    {
        if ((pc->type == CT_RETURN) && ((pc->flags & PCF_IN_PREPROC) == 0))
        {
            prev_pc = chunk_get_prev_nnl(pc);
            chunk_t * semi =  chunk_get_next(pc); /*normal has a semi or other*/
            next_pr = chunk_get_next_nnl(semi);
            
            prev_prev_pc = chunk_get_prev_nc(prev_pc);
            next_prev_pc = chunk_get_next_nc(prev_pc);
            
            prev_next_pc = chunk_get_prev_nc(next_pr);
            next_next_pc = chunk_get_next_nc(next_pr);
            
            if (true) {
                
            }
            
            if ((prev_prev_pc != NULL) && next_prev_pc != NULL && prev_next_pc != NULL &&
                next_next_pc !=NULL && (prev_pc != NULL) && (prev_pc->type == CT_BRACE_OPEN) &&
                (next_pr != NULL) && (next_pr->type == CT_BRACE_CLOSE) &&
                ((next_pr->parent_type != CT_FUNC_DEF) &&
                 (next_pr->parent_type != CT_FUNC_CLASS_DEF)) &&
                (pc->brace_level > 1))
            {
                LOG_FMT(LRMRETURN, "Removed  brace at single return on line %d\n", pc->orig_line);
               
                prev_prev_pc->next = next_prev_pc;
                prev_next_pc->next = next_next_pc;

                chunk_t *newline = newline_add_between(next_prev_pc, pc);
                if (newline &&  (newline->nl_count > 1))
                {
                    newline->nl_count = 1;
                }
                
                pc->indent.ref = pc;
                pc->indent.delta = 4;

                chunk_del(prev_pc);
                chunk_del(next_pr);
                pc = next_next_pc;
            }
        }
        
        pc = chunk_get_next(pc);
    }
}


/**
 * A func wrap chunk and what follows should be treated as a function name.
 * Create new text for the chunk and call it a CT_FUNCTION.
 *
 * A type wrap chunk and what follows should be treated as a simple type.
 * Create new text for the chunk and call it a CT_TYPE.
 */
static void handle_wrap(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *opp  = chunk_get_next(pc);
   chunk_t *name = chunk_get_next(opp);
   chunk_t *clp  = chunk_get_next(name);

   argval_t pav = (pc->type == CT_FUNC_WRAP) ?
                  cpd.settings[UO_sp_func_call_paren].a :
                  cpd.settings[UO_sp_cpp_cast_paren].a;

   argval_t av = (pc->type == CT_FUNC_WRAP) ?
                 cpd.settings[UO_sp_inside_fparen].a :
                 cpd.settings[UO_sp_inside_paren_cast].a;

   if ((clp != NULL) &&
       (opp->type == CT_PAREN_OPEN) &&
       ((name->type == CT_WORD) || (name->type == CT_TYPE)) &&
       (clp->type == CT_PAREN_CLOSE))
   {
      const char *psp = (pav & AV_ADD) ? " " : "";
      const char *fsp = (av & AV_ADD) ? " " : "";

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
}


/**
 * A proto wrap chunk and what follows should be treated as a function proto.
 *
 * RETTYPE PROTO_WRAP( NAME, PARAMS );
 * RETTYPE gets changed with make_type().
 * PROTO_WRAP is marked as CT_FUNC_PROTO or CT_FUNC_DEF.
 * NAME is marked as CT_WORD.
 * PARAMS is all marked as prototype parameters.
 */
static void handle_proto_wrap(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   chunk_t *opp  = chunk_get_next_ncnl(pc);
   chunk_t *name = chunk_get_next_ncnl(opp);
   chunk_t *tmp  = chunk_get_next_ncnl(chunk_get_next_ncnl(name));
   chunk_t *clp  = chunk_skip_to_match(opp);
   chunk_t *cma  = chunk_get_next_ncnl(clp);

   if (!opp || !name || !clp || !cma || !tmp ||
       ((name->type != CT_WORD) && (name->type != CT_TYPE)) ||
       (tmp->type != CT_PAREN_OPEN) ||
       (opp->type != CT_PAREN_OPEN))
   {
      return;
   }
   if (cma->type == CT_SEMICOLON)
   {
      set_chunk_type(pc, CT_FUNC_PROTO);
   }
   else if (cma->type == CT_BRACE_OPEN)
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
   fix_fcn_def_params(tmp);
   tmp = chunk_skip_to_match(tmp);
   if (tmp)
   {
      set_chunk_parent(tmp, CT_PROTO_WRAP);
   }

   /* Mark return type (TODO: move to own function) */
   tmp = pc;
   while ((tmp = chunk_get_prev_ncnl(tmp)) != NULL)
   {
      if (!chunk_is_type(tmp) &&
          (tmp->type != CT_OPERATOR) &&
          (tmp->type != CT_WORD) &&
          (tmp->type != CT_ADDR))
      {
         break;
      }
      set_chunk_parent(tmp, pc->type);
      make_type(tmp);
   }
}


/**
 * Java assert statments are: "assert EXP1 [: EXP2] ;"
 * Mark the parent of the colon and semicolon
 */
static void handle_java_assert(chunk_t *pc)
{
   LOG_FUNC_ENTRY();
   bool    did_colon = false;
   chunk_t *tmp      = pc;

   while ((tmp = chunk_get_next(tmp)) != NULL)
   {
      if (tmp->level == pc->level)
      {
         if (!did_colon && (tmp->type == CT_COLON))
         {
            did_colon = true;
            set_chunk_parent(tmp, pc->type);
         }
         if (tmp->type == CT_SEMICOLON)
         {
            set_chunk_parent(tmp, pc->type);
            break;
         }
      }
   }
}
