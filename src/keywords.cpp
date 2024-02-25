/**
 * @file keywords.cpp
 * Manages the table of keywords.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2023
 * @license GPL v2+
 */

#include "keywords.h"

#include "args.h"
#include "prototypes.h"
#include "uncrustify.h"
#include "uncrustify_limits.h"
//# define DEBUG_LANGUAGE
// see also the call to dump_keyword_for_lang
#ifdef DEBUG_LANGUAGE
#include "unc_tools.h"
#endif

#include <cerrno>
#include <map>


using namespace std;

// Dynamic keyword map
typedef map<string, E_Token> dkwmap;
static dkwmap dkwm;


/**
 * Compares two chunk_tag_t entries using strcmp on the strings
 *
 * @param the 'left' entry
 * @param the 'right' entry
 *
 * @return == 0  if both keywords are equal
 * @return  < 0  p1 is smaller than p2
 * @return  > 0  p2 is smaller than p1
 */
static int kw_compare(const void *p1, const void *p2);


/**
 * search in static keywords for first occurrence of a given tag
 *
 * @param tag/keyword to search for
 */
static const chunk_tag_t *kw_static_first(const chunk_tag_t *tag);


static const chunk_tag_t *kw_static_match(bool orig_list, const chunk_tag_t *tag, int lang_flags);

/**
 * selected keywords for the chosen language.
 */

static chunk_tag_t keyword_for_lang[uncrustify::limits::MAX_KEYWORDS];
static size_t      language_count;


/**
 * interesting static keywords - keep sorted.
 * Table includes the Name, Type, and Language flags.
 */
#include "chunk_tag_t_keywords.h"


// Issue #3353
void init_keywords_for_language()
{
   unsigned int local_flags    = cpd.lang_flags;
   size_t       keywords_count = ARRAY_SIZE(keywords);

   language_count = 0;

   for (size_t idx = 0; idx < keywords_count; idx++)
   {
      chunk_tag_t *tag = &keywords[idx];

      if ((tag->lang_flags & local_flags) != 0)
      {
         // for debugging only
         // fprintf(stderr, "%s(%d): %zu Keyword: '%s', type is '%s'\n",
         //         __func__, __LINE__, idx, tag->tag, get_token_name(tag->type));
         keyword_for_lang[language_count].tag        = tag->tag;
         keyword_for_lang[language_count].type       = tag->type;
         keyword_for_lang[language_count].lang_flags = tag->lang_flags;
         language_count++;
      }
   }

#ifdef DEBUG_LANGUAGE
   dump_keyword_for_lang(language_count, keyword_for_lang);
#endif

   LOG_FMT(LDYNKW, "%s(%d): Number of Keywords for language %s: '%zu'\n",
           __func__, __LINE__, language_name_from_flags(local_flags), language_count);
} // init_keywords_for_language


static int kw_compare(const void *p1, const void *p2)
{
   const chunk_tag_t *t1 = static_cast<const chunk_tag_t *>(p1);
   const chunk_tag_t *t2 = static_cast<const chunk_tag_t *>(p2);

   return(strcmp(t1->tag, t2->tag));
} // kw_compare


bool keywords_are_sorted()
{
   size_t keywords_count = ARRAY_SIZE(keywords);

   for (size_t idx = 1; idx < keywords_count; idx++)
   {
      if (kw_compare(&keywords[idx - 1], &keywords[idx]) > 0)
      {
         fprintf(stderr, "%s: bad sort order at idx %d, words '%s' and '%s'\n",
                 __func__, (int)idx - 1, keywords[idx - 1].tag, keywords[idx].tag);
         // coveralls will always complain.
         // these lines are only needed for the developer.
         log_flush(true);
         exit(EX_SOFTWARE);
      }
   }

   return(true);
} // keywords_are_sorted


void add_keyword(const std::string &tag, E_Token type)
{
   // See if the keyword has already been added
   dkwmap::iterator it = dkwm.find(tag);

   if (it != dkwm.end())
   {
      LOG_FMT(LDYNKW, "%s(%d): changed '%s' to '%s'\n",
              __func__, __LINE__, tag.c_str(), get_token_name(type));
      (*it).second = type;
      return;
   }
   // Insert the keyword
   dkwm.insert(dkwmap::value_type(tag, type));
   LOG_FMT(LDYNKW, "%s(%d): added '%s' as '%s'\n",
           __func__, __LINE__, tag.c_str(), get_token_name(type));
} // add_keyword


static const chunk_tag_t *kw_static_first(const chunk_tag_t *tag)
{
   const chunk_tag_t *prev = tag - 1;

   // TODO: avoid pointer arithmetic
   // loop over static keyword array
   while (  prev >= &keyword_for_lang[0]        // not at beginning of keyword array
         && strcmp(prev->tag, tag->tag) == 0)   // tags match
   {
      tag = prev;
      prev--;
   }
   return(tag);
} // kw_static_first


static const chunk_tag_t *kw_static_match(bool orig_list, const chunk_tag_t *tag, int lang_flags)
{
   bool in_pp = (  cpd.in_preproc != CT_NONE
                && cpd.in_preproc != CT_PP_DEFINE);

   for (const chunk_tag_t *iter = kw_static_first(tag);
        (orig_list) ? (iter < &keywords[ARRAY_SIZE(keywords)]) : (iter < &keyword_for_lang[language_count]);
        iter++)
   {
      bool        pp_iter = (iter->lang_flags & e_FLAG_PP) != 0; // forcing value to bool
      lang_flag_e temp    = (lang_flag_e)iter->lang_flags;

      if (  (strcmp(iter->tag, tag->tag) == 0)
         && language_is_set(temp)
         && (lang_flags & iter->lang_flags)
         && in_pp == pp_iter)
      {
         return(iter);
      }
   }

   return(nullptr);
} // kw_static_match


E_Token find_keyword_type(const char *word, size_t len)
{
   if (len <= 0)
   {
      return(CT_NONE);
   }
   // check the dynamic word list first
   string           ss(word, len);
   dkwmap::iterator it = dkwm.find(ss);

   if (it != dkwm.end())
   {
      return((*it).second);
   }
   chunk_tag_t key;

   key.tag = ss.c_str();

   // check the static word list
   const chunk_tag_t *p_ret = static_cast<const chunk_tag_t *>(
      bsearch(&key, keyword_for_lang, language_count, sizeof(keyword_for_lang[0]), kw_compare));

   if (p_ret != nullptr)
   {
      if (  strcmp(p_ret->tag, "__pragma") == 0
         || strcmp(p_ret->tag, "_Pragma") == 0)
      {
         cpd.in_preproc = CT_PREPROC;
      }
      p_ret = kw_static_match(false, p_ret, cpd.lang_flags);
   }
   return((p_ret != nullptr) ? p_ret->type : CT_WORD);
} // find_keyword_type


int load_keyword_file(const char *filename)
{
   FILE *pf = fopen(filename, "r");

   if (pf == nullptr)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      exit(EX_IOERR);
   }
   const int max_line_length = 256;
   const int max_arg_count   = 2;

   // maximal length of a line in the file
   char   buf[max_line_length];
   char   *args[max_arg_count];
   size_t line_no = 0;

   // read file line by line
   while (fgets(buf, max_line_length, pf) != nullptr)
   {
      line_no++;

      // remove comments after '#' sign
      char *ptr;

      if ((ptr = strchr(buf, '#')) != nullptr)
      {
         *ptr = 0; // set string end where comment begins
      }
      size_t argc = Args::SplitLine(buf, args, max_arg_count);

      if (argc > 0)
      {
         if (  argc == 1
            && CharTable::IsKw1(*args[0]))
         {
            add_keyword(args[0], CT_TYPE);
         }
         else
         {
            LOG_FMT(LWARN, "%s:%zu Invalid line (starts with '%s')\n",
                    filename, line_no, args[0]);
            exit(EX_SOFTWARE);
         }
      }
      else
      {
         continue; // the line is empty
      }
   }
   fclose(pf);
   return(EX_OK);
} // load_keyword_file


void print_custom_keywords(FILE *pfile)
{
   for (const auto &keyword_pair : dkwm)
   {
      E_Token tt = keyword_pair.second;

      if (tt == CT_TYPE)
      {
         fprintf(pfile, "custom type %*.s%s\n",
                 uncrustify::limits::MAX_OPTION_NAME_LEN - 10, " ",
                 keyword_pair.first.c_str());
      }
      else if (tt == CT_MACRO_OPEN)
      {
         fprintf(pfile, "macro-open %*.s%s\n",
                 uncrustify::limits::MAX_OPTION_NAME_LEN - 11, " ",
                 keyword_pair.first.c_str());
      }
      else if (tt == CT_MACRO_CLOSE)
      {
         fprintf(pfile, "macro-close %*.s%s\n",
                 uncrustify::limits::MAX_OPTION_NAME_LEN - 12, " ",
                 keyword_pair.first.c_str());
      }
      else if (tt == CT_MACRO_ELSE)
      {
         fprintf(pfile, "macro-else %*.s%s\n",
                 uncrustify::limits::MAX_OPTION_NAME_LEN - 11, " ",
                 keyword_pair.first.c_str());
      }
      else
      {
         const char *tn = get_token_name(tt);

         fprintf(pfile, "set %s %*.s%s\n",
                 tn,
                 uncrustify::limits::MAX_OPTION_NAME_LEN - (4 + static_cast<int>(strlen(tn))),
                 " ", keyword_pair.first.c_str());
      }
   }
} // print_custom_keywords


void clear_keyword_file()
{
   dkwm.clear();
} // clear_keyword_file


pattern_class_e get_token_pattern_class(E_Token tok)
{
   // TODO: instead of this switch better assign the pattern class to each statement
   switch (tok)
   {
   case CT_IF:
   case CT_ELSEIF:
   case CT_SWITCH:
   case CT_FOR:
   case CT_WHILE:
   case CT_SYNCHRONIZED:
   case CT_USING_STMT:
   case CT_LOCK:
   case CT_D_WITH:
   case CT_D_VERSION_IF:
   case CT_D_SCOPE_IF:
      return(pattern_class_e::PBRACED);

   case CT_ELSE:
      return(pattern_class_e::ELSE);

   case CT_DO:
   case CT_TRY:
   case CT_FINALLY:
   case CT_BODY:
   case CT_UNITTEST:
   case CT_UNSAFE:
   case CT_VOLATILE:
   case CT_GETSET:
      return(pattern_class_e::BRACED);

   case CT_CATCH:
   case CT_D_VERSION:
   case CT_DEBUG:
      return(pattern_class_e::OPBRACED);

   case CT_NAMESPACE:
      return(pattern_class_e::VBRACED);

   case CT_WHILE_OF_DO:
      return(pattern_class_e::PAREN);

   case CT_INVARIANT:
      return(pattern_class_e::OPPAREN);

   default:
      return(pattern_class_e::NONE);
   } // switch
}    // get_token_pattern_class
