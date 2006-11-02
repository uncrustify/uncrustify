/**
 * @file chunk_list.c
 * Manages and navigates the list of chunks.
 *
 * $Id$
 */

#ifndef CHUNK_LIST_H_INCLUDED
#define CHUNK_LIST_H_INCLUDED

#include "uncrustify_types.h"


chunk_t *chunk_add(const chunk_t *pc_in);
chunk_t *chunk_add_after(const chunk_t *pc_in, chunk_t *ref);
chunk_t *chunk_add_before(const chunk_t *pc_in, chunk_t *ref);

void chunk_del(chunk_t *pc);
void chunk_move_after(chunk_t *pc_in, chunk_t *ref);

chunk_t *chunk_get_head(void);
chunk_t *chunk_get_tail(void);
chunk_t *chunk_get_next(chunk_t *cur);
chunk_t *chunk_get_prev(chunk_t *cur);

void chunk_swap(chunk_t *pc1, chunk_t *pc2);

chunk_t *chunk_get_next_nl(chunk_t *cur);
chunk_t *chunk_get_next_nc(chunk_t *cur);
chunk_t *chunk_get_next_nnl(chunk_t *cur);
chunk_t *chunk_get_next_ncnl(chunk_t *cur);
chunk_t *chunk_get_next_ncnlnp(chunk_t *cur);

chunk_t *chunk_get_next_nblank(chunk_t *cur);
chunk_t *chunk_get_prev_nblank(chunk_t *cur);

chunk_t *chunk_get_prev_nl(chunk_t *cur);
chunk_t *chunk_get_prev_nc(chunk_t *cur);
chunk_t *chunk_get_prev_nnl(chunk_t *cur);
chunk_t *chunk_get_prev_ncnl(chunk_t *cur);
chunk_t *chunk_get_prev_ncnlnp(chunk_t *cur);

chunk_t *chunk_get_next_type(chunk_t *cur, c_token_t type, int level);
chunk_t *chunk_get_prev_type(chunk_t *cur, c_token_t type, int level);

chunk_t *chunk_get_next_str(chunk_t *cur, const char *str, int len, int level);
chunk_t *chunk_get_prev_str(chunk_t *cur, const char *str, int len, int level);

/**
 * Skips to the closing match for the current paren/brace/square.
 *
 * @param cur  The opening paren/brace/square
 * @return     NULL or the matching paren/brace/square
 */
static_inline
chunk_t *chunk_skip_to_match(chunk_t *cur)
{
   if ((cur != NULL) &&
       ((cur->type == CT_PAREN_OPEN) ||
        (cur->type == CT_SPAREN_OPEN) ||
        (cur->type == CT_FPAREN_OPEN) ||
        (cur->type == CT_BRACE_OPEN) ||
        (cur->type == CT_VBRACE_OPEN) ||
        (cur->type == CT_SQUARE_OPEN)))
   {
      return(chunk_get_next_type(cur, (c_token_t)(cur->type + 1), cur->level));
   }
   return(cur);
}


static_inline
bool chunk_is_comment(chunk_t *pc)
{
   return((pc != NULL) && ((pc->type == CT_COMMENT) ||
                           (pc->type == CT_COMMENT_MULTI) ||
                           (pc->type == CT_COMMENT_CPP)));
}

static_inline
bool chunk_is_newline(chunk_t *pc)
{
   return((pc != NULL) && ((pc->type == CT_NEWLINE) ||
                           (pc->type == CT_NL_CONT)));
}

static_inline
bool chunk_is_semicolon(chunk_t *pc)
{
   return((pc != NULL) && ((pc->type == CT_SEMICOLON) ||
                           (pc->type == CT_VSEMICOLON)));
}

static_inline
bool chunk_is_blank(chunk_t *pc)
{
   return((pc != NULL) && (pc->len == 0));
}

static_inline
bool chunk_is_preproc(chunk_t *pc)
{
   return((pc != NULL) && ((pc->flags & PCF_IN_PREPROC) != 0));
}

static_inline
bool chunk_is_type(chunk_t *pc)
{
   return((pc != NULL) && ((pc->type == CT_TYPE) ||
                           (pc->type == CT_PTR_TYPE) ||
                           (pc->type == CT_BYREF) ||
                           (pc->type == CT_DC_MEMBER) ||
                           (pc->type == CT_QUALIFIER) ||
                           (pc->type == CT_STRUCT) ||
                           (pc->type == CT_ENUM) ||
                           (pc->type == CT_UNION)));
}

static_inline
bool chunk_is_str(chunk_t *pc, const char *str, int len)
{
   return((pc != NULL) && (pc->len == len) && (memcmp(pc->str, str, len) == 0));
}

static_inline
bool chunk_is_star(chunk_t *pc)
{
   return((pc != NULL) && (pc->len == 1) && (pc->str[0] == '*'));
}

static_inline
bool chunk_is_addr(chunk_t *pc)
{
   return((pc != NULL) && (pc->len == 1) && (pc->str[0] == '&'));
}


bool chunk_is_newline_between(chunk_t *start, chunk_t *end);


#endif   /* CHUNK_LIST_H_INCLUDED */
