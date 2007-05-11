/**
 * @file defines.cpp
 * Manages the table of keywords.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */

#include "uncrustify_types.h"
#include "char_table.h"
#include "args.h"
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cctype>

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
         if ((argc <= 2) && ((get_char_table(*args[0]) & CT_KW1) != 0))
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

const define_tag_t *get_define_idx(int &idx)
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
