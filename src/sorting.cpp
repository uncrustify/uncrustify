/**
 * @file sorting.cpp
 * Sorts chunks and imports
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "sorting.h"

#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"
#include "newlines/add.h"
#include "newlines/double_newline.h"

#include <regex>
#include <unordered_map>


constexpr static auto LCURRENT = LSORT;


using namespace uncrustify;


Option<std::string> *include_category_options[] =
{
   &options::include_category_0,
   &options::include_category_1,
   &options::include_category_2,
};


constexpr static int kIncludeCategoriesCount = 3;


struct include_category
{
   include_category(const std::string &pattern)
      : regex(pattern)
   {
   }
   std::regex regex;
};


include_category                      *include_categories[kIncludeCategoriesCount];
std::unordered_map<Chunk *, int>      chunk_priority_cache;
std::unordered_map<std::string, bool> filename_without_ext_cache;


/**
 * Compare two series of chunks, starting with the given ones.
 * @param pc1   first  instance to compare
 * @param pc2   second instance to compare
 * @param tcare take care of case (lower case/ upper case)   Issue #2091
 *
 * @retval == 0  both text elements are equal
 * @retval  > 0
 * @retval  < 0
 */
static int compare_chunks(Chunk *pc1, Chunk *pc2, bool tcare);


/**
 * Sort all of the chunks in O(n log n) time with a maximum of O(n) swaps
 */
static void do_the_sort(Chunk **chunks, size_t num_chunks);


static void prepare_categories()
{
   for (int i = 0; i < kIncludeCategoriesCount; ++i)
   {
      const auto &cat_pattern = (*include_category_options[i])();

      if (!cat_pattern.empty())
      {
         include_categories[i] = new include_category(cat_pattern);
      }
      else
      {
         include_categories[i] = nullptr;
      }
   }
}


static void cleanup_categories()
{
   chunk_priority_cache.clear();
   filename_without_ext_cache.clear();

   for (auto &include_category : include_categories)
   {
      if (include_category == nullptr)
      {
         continue;
      }
      delete include_category;
      include_category = NULL;
   }
}


static int get_chunk_priority(Chunk *pc)
{
   if (chunk_priority_cache.count(pc) > 0)
   {
      return(chunk_priority_cache[pc]);
   }
   int category = kIncludeCategoriesCount;

   for (int i = 0; i < kIncludeCategoriesCount; i++)
   {
      if (include_categories[i] != nullptr)
      {
         if (std::regex_match(pc->Text(), include_categories[i]->regex))
         {
            category = i;
            break;
         }
      }
   }

   chunk_priority_cache[pc] = category;
   return(category);
}


/**
 * Returns true if the text contains filename without extension.
 */
static bool text_contains_filename_without_ext(const char *text)
{
   if (filename_without_ext_cache.count(text) > 0)
   {
      return(filename_without_ext_cache[text]);
   }
   std::string filepath             = cpd.filename;
   std::string filename_without_ext = filepath;
   size_t      startIndex           = filepath.find_last_of("/\\");

   if (startIndex == std::string::npos)
   {
      startIndex = 0;
   }
   else
   {
      startIndex += 1;
   }

   if (startIndex < filepath.size())
   {
      std::string filename = filepath.substr(startIndex);
      size_t      dot_idx  = filename.find_last_of('.');
      filename_without_ext = filename.substr(0, dot_idx);
   }
   const std::regex  special_chars      = std::regex(R"([-[\]{}()*+?.,\^$|#\s])");
   const std::string sanitized_filename = std::regex_replace(filename_without_ext, special_chars, R"(\$&)");
   const std::regex  filename_pattern   = std::regex("\\S?" + sanitized_filename + "\\b.*");

   filename_without_ext_cache[text] = std::regex_match(text, filename_pattern);
   return(filename_without_ext_cache[text]);
}


/**
 * Get chunk text without the extension.
 */
static UncText get_text_without_ext(const UncText &chunk_text)
{
   UncText result = chunk_text;
   int     idx    = result.rfind(".", result.size() - 1);

   if (idx == -1)
   {
      return(result);
   }
   result.erase(idx, result.size() - idx);
   return(result);
}


/**
 * Returns true if UncText has "." which implies extension.
 */
static bool has_dot(const UncText &chunk_text)
{
   int idx = chunk_text.rfind(".", chunk_text.size() - 1);

   return(idx != -1);
}


/**
 * Returns chunk string required for sorting.
 */
static UncText chunk_sort_str(Chunk *pc)
{
   if (pc->GetParentType() == CT_PP_INCLUDE)
   {
      return(UncText{ pc->GetStr(), 0, pc->Len() - 1 });
   }
   return(pc->GetStr());
}


//! Compare two chunks
static int compare_chunks(Chunk *pc1, Chunk *pc2, bool tcare)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSORT, "%s(%d): @begin pc1->len is %zu, line is %zu, column is %zu\n",
           __func__, __LINE__, pc1->Len(), pc1->GetOrigLine(), pc1->GetOrigCol());
   LOG_FMT(LSORT, "%s(%d): @begin pc2->len is %zu, line is %zu, column is %zu\n",
           __func__, __LINE__, pc2->Len(), pc2->GetOrigLine(), pc2->GetOrigCol());

   if (pc1 == pc2) // same chunk is always identical thus return 0 differences
   {
      return(0);
   }

   while (  pc1->IsNotNullChunk()
         && pc2->IsNotNullChunk())
   {
      auto const &s1_ext = chunk_sort_str(pc1);
      auto const &s2_ext = chunk_sort_str(pc2);

      log_rule_B("mod_sort_incl_import_ignore_extension");
      auto const &s1 = (options::mod_sort_incl_import_ignore_extension()) ? get_text_without_ext(s1_ext) : s1_ext;
      auto const &s2 = (options::mod_sort_incl_import_ignore_extension()) ? get_text_without_ext(s2_ext) : s2_ext;
      log_rule_B("mod_sort_incl_import_prioritize_filename");

      if (options::mod_sort_incl_import_prioritize_filename())
      {
         bool s1_contains_filename = text_contains_filename_without_ext(s1.c_str());
         bool s2_contains_filename = text_contains_filename_without_ext(s2.c_str());

         if (  s1_contains_filename
            && !s2_contains_filename)
         {
            return(-1);
         }
         else if (  !s1_contains_filename
                 && s2_contains_filename)
         {
            return(1);
         }
      }

      if (options::mod_sort_incl_import_prioritize_extensionless())
      {
         log_rule_B("mod_sort_incl_import_prioritize_extensionless");
         const bool s1_has_dot = has_dot(s1_ext);
         const bool s2_has_dot = has_dot(s2_ext);

         if (  s1_has_dot
            && !s2_has_dot)
         {
            return(1);
         }
         else if (  !s1_has_dot
                 && s2_has_dot)
         {
            return(-1);
         }
      }

      if (options::mod_sort_incl_import_prioritize_angle_over_quotes())
      {
         log_rule_B("mod_sort_incl_import_prioritize_angle_over_quotes");

         if (  s1.startswith("<")
            && s2.startswith("\""))
         {
            return(-1);
         }
         else if (  s1.startswith("\"")
                 && s2.startswith("<"))
         {
            return(1);
         }
      }
      int ppc1 = get_chunk_priority(pc1);
      int ppc2 = get_chunk_priority(pc2);

      if (ppc1 != ppc2)
      {
         return(ppc1 - ppc2);
      }
      LOG_FMT(LSORT, "%s(%d): text is %s, pc1->len is %zu, line is %zu, column is %zu\n",
              __func__, __LINE__, pc1->Text(), pc1->Len(), pc1->GetOrigLine(), pc1->GetOrigCol());
      LOG_FMT(LSORT, "%s(%d): text is %s, pc2->len is %zu, line is %zu, column is %zu\n",
              __func__, __LINE__, pc2->Text(), pc2->Len(), pc2->GetOrigLine(), pc2->GetOrigCol());

      int ret_val = UncText::compare(s1, s2, std::min(s1.size(), s2.size()), tcare);
      LOG_FMT(LSORT, "%s(%d): ret_val is %d\n",
              __func__, __LINE__, ret_val);

      if (ret_val != 0)
      {
         return(ret_val);
      }

      if (pc1->Len() != pc2->Len())
      {
         return(pc1->Len() - pc2->Len());
      }
      // Same word, same length. Step to the next chunk.
      pc1 = pc1->GetNext();
      LOG_FMT(LSORT, "%s(%d): text is %s, pc1->len is %zu, line is %zu, column is %zu\n",
              __func__, __LINE__, pc1->Text(), pc1->Len(), pc1->GetOrigLine(), pc1->GetOrigCol());

      if (pc1->Is(CT_MEMBER))
      {
         pc1 = pc1->GetNext();
         LOG_FMT(LSORT, "%s(%d): text is %s, pc1->len is %zu, line is %zu, column is %zu\n",
                 __func__, __LINE__, pc1->Text(), pc1->Len(), pc1->GetOrigLine(), pc1->GetOrigCol());
      }
      pc2 = pc2->GetNext();
      LOG_FMT(LSORT, "%s(%d): text is %s, pc2->len is %zu, line is %zu, column is %zu\n",
              __func__, __LINE__, pc2->Text(), pc2->Len(), pc2->GetOrigLine(), pc2->GetOrigCol());

      if (pc2->Is(CT_MEMBER))
      {
         pc2 = pc2->GetNext();
         LOG_FMT(LSORT, "%s(%d): text is %s, pc2->len is %zu, line is %zu, column is %zu\n",
                 __func__, __LINE__, pc2->Text(), pc2->Len(), pc2->GetOrigLine(), pc2->GetOrigCol());
      }
      LOG_FMT(LSORT, "%s(%d): >>>text is %s, pc1->len is %zu, line is %zu, column is %zu\n",
              __func__, __LINE__, pc1->Text(), pc1->Len(), pc1->GetOrigLine(), pc1->GetOrigCol());
      LOG_FMT(LSORT, "%s(%d): >>>text is %s, pc2->len is %zu, line is %zu, column is %zu\n",
              __func__, __LINE__, pc2->Text(), pc2->Len(), pc2->GetOrigLine(), pc2->GetOrigCol());

      // If we hit a newline or null chuck, we are done
      if (  pc1->IsNullChunk()
         || pc1->IsNewline()
         || pc2->IsNullChunk()
         || pc2->IsNewline())
      {
         break;
      }
   }

   if (  pc1->IsNullChunk()
      || !pc2->IsNewline())
   {
      return(-1);
   }

   if (!pc1->IsNewline())
   {
      return(1);
   }
   return(0);
} // compare_chunks


/**
 * Sort all of the chunks in O(n log n) time with a maximum of O(n) swaps
 */
static void do_the_sort(Chunk **chunks, size_t num_chunks)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSORT, "%s(%d): %zu chunks:",
           __func__, __LINE__, num_chunks);

   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      LOG_FMT(LSORT, " [%s]", chunks[idx]->Text());
   }

   LOG_FMT(LSORT, "\n");

   log_rule_B("mod_sort_case_sensitive");
   bool take_care = options::mod_sort_case_sensitive();                    // Issue #2091

   // Sort an array of the chunk positions in order to minimize the number of swaps
   std::vector<size_t> chunk_positions(num_chunks);

   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      chunk_positions[idx] = idx;
   }

   sort(chunk_positions.begin(), chunk_positions.end(), [chunks, take_care](const size_t &l, const size_t &r) {
      return(compare_chunks(chunks[l], chunks[r], take_care) < 0);
   });

   // Swap the chunk positions
   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      if (chunk_positions[idx] != idx)
      {
         log_rule_B("mod_sort_incl_import_grouping_enabled");

         const size_t from = chunk_positions[idx];
         const size_t to   = chunk_positions[from];
         chunks[from]->SwapLines(chunks[to]);

         Chunk *pc = chunks[from];
         chunks[from] = chunks[to];
         chunks[to]   = pc;

         chunk_positions[from] = from;
         chunk_positions[idx]  = to;
         idx--;
      }
   }
} // do_the_sort


/**
 * Remove blank lines between chunks.
 */
static void remove_blank_lines_between_imports(Chunk **chunks, size_t num_chunks)
{
   LOG_FUNC_ENTRY();

   if (num_chunks < 2)
   {
      return;
   }

   for (size_t idx = 0; idx < (num_chunks - 1); idx++)
   {
      Chunk *chunk1 = chunks[idx]->GetNextNl();
      chunk1->SetNlCount(1);
      MARK_CHANGE();
   }
}


/**
 * Delete chunks on line having chunk.
 */
static void delete_chunks_on_line_having_chunk(Chunk *chunk)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = chunk->GetFirstChunkOnLine();

   while (  pc->IsNotNullChunk()
         && !pc->IsComment())
   {
      Chunk *next_pc = pc->GetNext();
      LOG_FMT(LCHUNK, "%s(%d): Removed '%s' on orig line %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine());

      if (pc->IsNewline())
      {
         Chunk::Delete(pc);
         break;
      }
      else
      {
         Chunk::Delete(pc);
      }
      pc = next_pc;
   }
}


/**
 * Dedupe import/include directives.
 */
static void dedupe_imports(Chunk **chunks, size_t num_chunks)
{
   LOG_FUNC_ENTRY();
   log_rule_B("mod_sort_case_sensitive");

   for (size_t idx = 1; idx < num_chunks; idx++)
   {
      auto const &s1 = chunk_sort_str(chunks[idx - 1]);
      auto const &s2 = chunk_sort_str(chunks[idx]);

      if (s1.size() != s2.size())
      {
         continue;
      }
      int ret_val = UncText::compare(s1, s2, std::min(s1.size(), s2.size()), options::mod_sort_case_sensitive());

      if (ret_val == 0)
      {
         delete_chunks_on_line_having_chunk(chunks[idx - 1]);
      }
   }
}


/**
 * Add blank line before the chunk.
 */
static void blankline_add_before(Chunk *pc)
{
   Chunk *newline = newline_add_before(pc->GetFirstChunkOnLine());

   if (newline->GetNlCount() < 2)
   {
      double_newline(newline);
   }
}


/**
 * Group imports.
 */
static void group_imports_by_adding_newlines(Chunk **chunks, size_t num_chunks)
{
   LOG_FUNC_ENTRY();

   // Group imports based on first character, typically quote or angle.
   int c_idx      = -1;
   int c_idx_last = -1;

   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      if (chunks[idx]->GetStr().size() > 0)
      {
         c_idx = chunks[idx]->GetStr().at(0);
      }
      else
      {
         c_idx = -1;
      }

      if (  c_idx_last != c_idx
         && idx > 0)
      {
         blankline_add_before(chunks[idx]);
      }
      c_idx_last = c_idx;
   }

   // Group imports based on having extension.
   bool chunk_has_dot      = false;
   bool chunk_last_has_dot = false;

   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      chunk_has_dot = has_dot(chunks[idx]->GetStr());

      if (  chunk_last_has_dot != chunk_has_dot
         && idx > 0)
      {
         blankline_add_before(chunks[idx]);
      }
      chunk_last_has_dot = chunk_has_dot;
   }

   // Group imports based on priority defined by config.
   int chunk_pri      = -1;
   int chunk_pri_last = -1;

   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      chunk_pri = get_chunk_priority(chunks[idx]);

      if (  chunk_pri_last != chunk_pri
         && idx > 0)
      {
         blankline_add_before(chunks[idx]);
      }
      chunk_pri_last = chunk_pri;
   }

   // Group imports that contain filename pattern.
   bool chunk_has_filename      = false;
   bool last_chunk_has_filename = false;

   for (size_t idx = 0; idx < num_chunks; idx++)
   {
      auto const &chunk_text = chunk_sort_str(chunks[idx]);
      chunk_has_filename = text_contains_filename_without_ext(chunk_text.c_str());

      if (  !chunk_has_filename
         && last_chunk_has_filename)
      {
         blankline_add_before(chunks[idx]);
      }
      last_chunk_has_filename = chunk_has_filename;
   }
} // group_imports_by_adding_newlines


void sort_imports()
{
   LOG_FUNC_ENTRY();
   const int max_number_to_sort                        = 1024;
   const int max_lines_to_check_for_sort_after_include = 128;
   const int max_gap_threshold_between_include_to_sort = 32;

   Chunk     *chunks[max_number_to_sort];
   size_t    num_chunks  = 0;
   Chunk     *p_last     = Chunk::NullChunkPtr;
   Chunk     *p_imp      = Chunk::NullChunkPtr;
   Chunk     *p_imp_last = Chunk::NullChunkPtr;

   prepare_categories();

   Chunk *pc = Chunk::GetHead();

   log_rule_B("mod_sort_incl_import_grouping_enabled");

   while (pc->IsNotNullChunk())
   {
      // Simple optimization to limit the sorting. Any MAX_LINES_TO_CHECK_AFTER_INCLUDE lines after last
      // import is seen are ignore from sorting.
      if (  options::mod_sort_incl_import_grouping_enabled()
         && p_imp_last->IsNotNullChunk()
         && (pc->GetOrigLine() - p_imp_last->GetOrigLine()) > max_lines_to_check_for_sort_after_include)
      {
         break;
      }
      Chunk *next = pc->GetNext();

      if (pc->IsNewline())
      {
         bool did_import = false;

         if (  p_imp->IsNotNullChunk()
            && (  p_last->Is(CT_SEMICOLON)
               || p_imp->TestFlags(PCF_IN_PREPROC)))
         {
            if (num_chunks < max_number_to_sort)
            {
               LOG_FMT(LSORT, "%s(%d): p_imp is %s\n",
                       __func__, __LINE__, p_imp->Text());
               chunks[num_chunks++] = p_imp;
            }
            else
            {
               fprintf(stderr, "Number of 'import' to be sorted is too big for the current value %d.\n", max_number_to_sort);
               fprintf(stderr, "Please make a report.\n");
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            did_import = true;
         }
         log_rule_B("mod_sort_incl_import_grouping_enabled");

         if (  !did_import
            || (  !options::mod_sort_incl_import_grouping_enabled()
               && pc->GetNlCount() > 1)
            || (  options::mod_sort_incl_import_grouping_enabled()
               && p_imp_last->IsNotNullChunk()
               && (pc->GetOrigLine() - p_imp_last->GetOrigLine()) > max_gap_threshold_between_include_to_sort)
            || next->IsNullChunk())
         {
            if (num_chunks > 1)
            {
               log_rule_B("mod_sort_incl_import_grouping_enabled");

               if (options::mod_sort_incl_import_grouping_enabled())
               {
                  remove_blank_lines_between_imports(chunks, num_chunks);
                  do_the_sort(chunks, num_chunks);
                  group_imports_by_adding_newlines(chunks, num_chunks);
                  dedupe_imports(chunks, num_chunks);
               }
               else
               {
                  do_the_sort(chunks, num_chunks);
               }
            }
            num_chunks = 0;
         }
         p_imp_last = p_imp;
         p_imp      = Chunk::NullChunkPtr;
         p_last     = Chunk::NullChunkPtr;
      }
      else if (pc->Is(CT_IMPORT))
      {
         log_rule_B("mod_sort_import");

         if (options::mod_sort_import())
         {
            p_imp = pc->GetNext();
         }
      }
      else if (pc->Is(CT_USING))
      {
         log_rule_B("mod_sort_using");

         if (options::mod_sort_using())
         {
            p_imp = pc->GetNext();
         }
      }
      else if (pc->Is(CT_PP_INCLUDE))
      {
         log_rule_B("mod_sort_include");

         if (options::mod_sort_include())
         {
            p_imp  = pc->GetNext();
            p_last = pc;
         }
      }
      else if (!pc->IsComment())
      {
         p_last = pc;
      }
      pc = next;
   }
   cleanup_categories();
} // sort_imports
