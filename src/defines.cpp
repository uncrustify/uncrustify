/**
 * @file defines.cpp
 * Manages the table of keywords.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "char_table.h"
#include "args.h"
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include "unc_ctype.h"
#include "chunk_list.h"
#include "prototypes.h"

typedef struct
{
   define_tag_t *p_tags;
   int          total;            /* number of items at p_tags */
   int          active;           /* number of valid entries */
} define_list_t;
static define_list_t dl;


/**
 * Compares two define_tag_t entries using strcmp on the name
 *
 * @param p1   The 'left' entry
 * @param p2   The 'right' entry
 */
static int def_compare(const void *p1, const void *p2)
{
   const define_tag_t *t1 = (const define_tag_t *)p1;
   const define_tag_t *t2 = (const define_tag_t *)p2;

   return(strcmp(t1->tag, t2->tag));
}


/**
 * Adds an entry to the define list
 *
 * @param tag        The tag (string) must be zero terminated
 * @param value      NULL or the value of the define
 */
void add_define(const char *tag, const char *value)
{
   /* Update existing entry */
   if (dl.active > 0)
   {
      define_tag_t *p_ret;

      p_ret = (define_tag_t *)bsearch(&tag, dl.p_tags, dl.active,
                                      sizeof(define_tag_t), def_compare);
      if (p_ret != NULL)
      {
         if (*p_ret->value != 0)
         {
            free((void *)p_ret->value);
         }
         if ((value == NULL) || (*value == 0))
         {
            dl.p_tags[dl.active].value = "";
         }
         else
         {
            dl.p_tags[dl.active].value = strdup(value);
         }
         return;
      }
   }

   /* need to add it to the list: do we need to allocate more memory? */
   if ((dl.total == dl.active) || (dl.p_tags == NULL))
   {
      dl.total += 16;
      dl.p_tags = (define_tag_t *)realloc(dl.p_tags, sizeof(define_tag_t) * dl.total);
   }
   if (dl.p_tags != NULL)
   {
      /* add to the end of the list */
      dl.p_tags[dl.active].tag = strdup(tag);
      if ((value == NULL) || (*value == 0))
      {
         dl.p_tags[dl.active].value = "";
      }
      else
      {
         dl.p_tags[dl.active].value = strdup(value);
      }
      dl.active++;

      /* Todo: add in sorted order instead of resorting the whole list? */
      qsort(dl.p_tags, dl.active, sizeof(define_tag_t), def_compare);

      LOG_FMT(LDEFVAL, "%s: added '%s' = '%s'\n",
              __func__, tag, value ? value : "NULL");
   }
}


/**
 * Search the define table for a match
 *
 * @param word    Pointer to the text -- NOT zero terminated
 * @param len     The length of the text
 * @return        NULL (no match) or the define entry
 */
const define_tag_t *find_define(const char *word, int len)
{
   define_tag_t       tag;
   char               buf[32];
   const define_tag_t *p_ret;

   if (len > (int)(sizeof(buf) - 1))
   {
      LOG_FMT(LNOTE, "%s: define too long at %d char (%d max) : %.*s\n",
              __func__, len, (int)sizeof(buf), len, word);
      return(NULL);
   }
   memcpy(buf, word, len);
   buf[len] = 0;

   tag.tag = buf;

   /* check the dynamic word list first */
   p_ret = (const define_tag_t *)bsearch(&tag, dl.p_tags, dl.active,
                                         sizeof(define_tag_t), def_compare);
   return(p_ret);
}


/**
 * Loads the defines from a file
 *
 * @param filename   The path to the file to load
 * @return           SUCCESS or FAILURE
 */
int load_define_file(const char *filename)
{
   FILE *pf;
   char buf[160];
   char *ptr;
   char *args[3];
   int  argc;
   int  line_no = 0;

   pf = fopen(filename, "r");
   if (pf == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
      return(FAILURE);
   }

   while (fgets(buf, sizeof(buf), pf) != NULL)
   {
      line_no++;

      /* remove comments */
      if ((ptr = strchr(buf, '#')) != NULL)
      {
         *ptr = 0;
      }

      argc       = Args::SplitLine(buf, args, ARRAY_SIZE(args) - 1);
      args[argc] = 0;

      if (argc > 0)
      {
         if ((argc <= 2) && CharTable::IsKw1(*args[0]))
         {
            LOG_FMT(LDEFVAL, "%s: line %d - %s\n", filename, line_no, args[0]);
            add_define(args[0], args[1]);
         }
         else
         {
            LOG_FMT(LWARN, "%s: line %d invalid (starts with '%s')\n",
                    filename, line_no, args[0]);
            cpd.error_count++;
         }
      }
   }

   fclose(pf);
   return(SUCCESS);
}


void output_defines(FILE *pfile)
{
   int idx;

   if (dl.active > 0)
   {
      fprintf(pfile, "-== User Defines ==-\n");
   }
   for (idx = 0; idx < dl.active; idx++)
   {
      if (*dl.p_tags[idx].value != 0)
      {
         fprintf(pfile, "%s = %s\n", dl.p_tags[idx].tag, dl.p_tags[idx].value);
      }
      else
      {
         fprintf(pfile, "%s\n", dl.p_tags[idx].tag);
      }
   }
}


const define_tag_t *get_define_idx(int& idx)
{
   const define_tag_t *dt = NULL;

   if ((idx >= 0) && (idx < dl.active))
   {
      dt = &dl.p_tags[idx];
   }
   idx++;
   return(dt);
}


void clear_defines(void)
{
   if (dl.p_tags != NULL)
   {
      for (int idx = 0; idx < dl.active; idx++)
      {
         free((void *)dl.p_tags[idx].tag);
         dl.p_tags[idx].tag = NULL;
         if (dl.p_tags[idx].value != NULL)
         {
            free((void *)dl.p_tags[idx].value);
            dl.p_tags[idx].value = NULL;
         }
      }
      free(dl.p_tags);
      dl.p_tags = NULL;
   }
   dl.total  = 0;
   dl.active = 0;
}


/**
 * This renders the #if condition to a string buffer.
 */
static void generate_if_conditional_as_text(std::string& dst, chunk_t *ifdef)
{
   chunk_t *pc;
   int     column = -1;

   dst.erase();
   for (pc = ifdef; pc != NULL; pc = chunk_get_next(pc))
   {
      if (column == -1)
      {
         column = pc->column;
      }
      if ((pc->type == CT_NEWLINE) ||
          (pc->type == CT_COMMENT_MULTI) ||
          (pc->type == CT_COMMENT_CPP))
      {
         break;
      }
      else if (pc->type == CT_NL_CONT)
      {
         dst   += ' ';
         column = -1;
      }
      else if ((pc->type == CT_COMMENT) ||
               (pc->type == CT_COMMENT_EMBED))
      {
      }
      else // if (pc->type == CT_JUNK) || else
      {
         int spacing;

         for (spacing = pc->column - column; spacing > 0; spacing--)
         {
            dst += ' ';
            column++;
         }
         dst.append(pc->str, pc->len);
         column += pc->len;
      }
   }
}


/*
 * See also it's preprocessor counterpart
 *   add_long_closebrace_comment
 * in braces.cpp
 *
 * Note: since this concerns itself with the preprocessor -- which is line-oriented --
 * it turns out that just looking at pc->pp_level is NOT the right thing to do.
 * See a --parsed dump if you don't believe this: an '#endif' will be one level
 * UP from the corresponding #ifdef when you look at the tokens 'ifdef' versus 'endif',
 * but it's a whole another story when you look at their CT_PREPROC ('#') tokens!
 *
 * Hence we need to track and seek matching CT_PREPROC pp_levels here, which complicates
 * things a little bit, but not much.
 */
void add_long_preprocessor_conditional_block_comment(void)
{
   chunk_t *pc;
   chunk_t *tmp;
   chunk_t *br_open;
   chunk_t *br_close;
   chunk_t *pp_start = NULL;
   chunk_t *pp_end   = NULL;
   int     nl_count;

   for (pc = chunk_get_head(); pc; pc = chunk_get_next_ncnl(pc))
   {
      /* just track the preproc level: */
      if (pc->type == CT_PREPROC)
      {
         pp_end = pp_start = pc;
      }

      if (pc->type != CT_PP_IF)
      {
         continue;
      }
#if 0
      if ((pc->flags & PCF_IN_PREPROC) != 0)
      {
         continue;
      }
#endif

      br_open  = pc;
      nl_count = 0;

      tmp = pc;
      while ((tmp = chunk_get_next(tmp)) != NULL)
      {
         /* just track the preproc level: */
         if (tmp->type == CT_PREPROC)
         {
            pp_end = tmp;
         }

         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
         }
         else if ((pp_end->pp_level == pp_start->pp_level) &&
                  ((tmp->type == CT_PP_ENDIF) ||
                   (br_open->type == CT_PP_IF ? tmp->type == CT_PP_ELSE : 0)))
         {
            br_close = tmp;

            LOG_FMT(LPPIF, "found #if / %s section on lines %d and %d, nl_count=%d\n",
                    (tmp->type == CT_PP_ENDIF ? "#endif" : "#else"),
                    br_open->orig_line, br_close->orig_line, nl_count);

            /* Found the matching #else or #endif - make sure a newline is next */
            tmp = chunk_get_next(tmp);

            LOG_FMT(LPPIF, "next item type %d (is %s)\n",
                    (tmp ? tmp->type : -1), (tmp ? chunk_is_newline(tmp) ? "newline"
                                             : chunk_is_comment(tmp) ? "comment" : "other" : "---"));
            if ((tmp == NULL) || (tmp->type == CT_NEWLINE) /* chunk_is_newline(tmp) */)
            {
               int nl_min;

               if (br_close->type == CT_PP_ENDIF)
               {
                  nl_min = cpd.settings[UO_mod_add_long_ifdef_endif_comment].n;
               }
               else
               {
                  nl_min = cpd.settings[UO_mod_add_long_ifdef_else_comment].n;
               }

               LOG_FMT(LPPIF, "#if / %s section candidate for augmenting when over NL threshold %d != 0 (nl_count=%d)\n",
                       (tmp->type == CT_PP_ENDIF ? "#endif" : "#else"),
                       nl_min, nl_count);

               if ((nl_min > 0) && (nl_count > nl_min)) /* nl_count is 1 too large at all times as #if line was counted too */
               {
                  /* determine the added comment style */
                  c_token_t style = (cpd.lang_flags & (LANG_CPP | LANG_CS)) ?
                                    CT_COMMENT_CPP : CT_COMMENT;

                  std::string str;
                  generate_if_conditional_as_text(str, br_open);

                  LOG_FMT(LPPIF, "#if / %s section over threshold %d (nl_count=%d) --> insert comment after the %s: %s\n",
                          (tmp->type == CT_PP_ENDIF ? "#endif" : "#else"),
                          nl_min, nl_count,
                          (tmp->type == CT_PP_ENDIF ? "#endif" : "#else"),
                          str.c_str());

                  /* Add a comment after the close brace */
                  insert_comment_after(br_close, style, str.length(), str.c_str());
               }
            }

            /* checks both the #else and #endif for a given level, only then look further in the main loop */
            if (br_close->type == CT_PP_ENDIF)
            {
               break;
            }
         }
      }
   }
}
