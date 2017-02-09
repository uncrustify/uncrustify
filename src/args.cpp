/**
 * @file args.cpp
 * Parses command line arguments.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "args.h"
#include <cstring>
#include "unc_ctype.h"


/**
 * Store the values and allocate enough memory for the 'used' flags.
 *
 * @param argc The argc that was passed to main()
 * @param argv The argv that was passed to main()
 */
Args::Args(int argc, char **argv)
{
   m_count  = argc;
   m_values = argv;
   size_t len = (argc >> 3) + 1;
   m_used = new UINT8[len];
   if (m_used != nullptr)
   {
      memset(m_used, 0, len);
   }
}


Args::~Args()
{
   if (m_used != nullptr)
   {
      delete[] m_used;
      m_used = nullptr;
   }
   m_count = 0;
}


/**
 * Check for an exact match
 *
 * @param token   The token string to match
 * @return        true/false -- Whether the argument was present
 */
bool Args::Present(const char *token)
{
   if (token != nullptr)
   {
      for (size_t idx = 0; idx < m_count; idx++)
      {
         if (strcmp(token, m_values[idx]) == 0)
         {
            SetUsed(idx);
            return(true);
         }
      }
   }
   return(false);
}


/**
 * Just call arg_params() with an index of 0.
 *
 * @param token   The token string to match
 * @return        NULL or the pointer to the string
 */
const char *Args::Param(const char *token)
{
   size_t idx = 0;

   return(Params(token, idx));
}


/**
 * Scan for a match
 *
 * @param token   The token string to match
 * @return        NULL or the pointer to the string
 */
const char *Args::Params(const char *token, size_t &index)
{
   if (token == nullptr)
   {
      return(nullptr);
   }

   size_t token_len = strlen(token);

   for (size_t idx = index; idx < m_count; idx++)
   {
      size_t arg_len = strlen(m_values[idx]);

      if ((arg_len >= token_len) &&
          (memcmp(token, m_values[idx], token_len) == 0))
      {
         SetUsed(idx);
         if (arg_len > token_len)
         {
            if (m_values[idx][token_len] == '=')
            {
               token_len++;
            }
            index = idx + 1;
            return(&m_values[idx][token_len]);
         }
         idx++;
         index = idx + 1;
         if (idx < m_count)
         {
            SetUsed(idx);
            return(m_values[idx]);
         }
         return("");
      }
   }

   return(nullptr);
} // Args::Params


/**
 * Gets whether an argument has been used, by index.
 *
 * @param idx  The index of the argument
 */
bool Args::GetUsed(size_t idx)
{
   if ((m_used != nullptr) && (idx > 0) && (idx < m_count))
   {
      return((m_used[idx >> 3] & (1 << (idx & 0x07))) != 0);
   }
   return(false);
}


/**
 * Marks an argument as being used.
 *
 * @param idx  The index of the argument
 */
void Args::SetUsed(size_t idx)
{
   if ((m_used != nullptr) && (idx > 0) && (idx < m_count))
   {
      m_used[idx >> 3] |= (1 << (idx & 0x07));
   }
}


/**
 * This function retrieves all unused parameters.
 * You must set the index before the first call.
 * Set the index to 1 to skip argv[0].
 *
 * @param idx  Pointer to the index
 * @return     NULL (done) or the pointer to the string
 */
const char *Args::Unused(size_t &index)
{
   if (m_used == nullptr)
   {
      return(nullptr);
   }

   for (size_t idx = index; idx < m_count; idx++)
   {
      if (!GetUsed(idx))
      {
         index = idx + 1;
         return(m_values[idx]);
      }
   }
   index = m_count;
   return(nullptr);
}


/**
 * Takes text and splits it into arguments.
 * args is an array of char * pointers that will get populated.
 * num_args is the maximum number of args split off.
 * If there are more than num_args, the remaining text is ignored.
 * Note that text is modified (zeroes are inserted)
 *
 * @param text       The text to split (modified)
 * @param args       The char * array to populate
 * @param num_args   The number of items in args
 * @return           The number of arguments parsed (always <= num_args)
 */
size_t Args::SplitLine(char *text, char *args[], size_t num_args)
{
   char   cur_quote    = 0;
   bool   in_backslash = false;
   bool   in_arg       = false;
   size_t argc         = 0;
   char   *dest        = text;


   while ((*text != 0) && (argc <= num_args))
   {
      /* Detect the start of an arg */
      if (!in_arg && !unc_isspace(*text))
      {
         in_arg     = true;
         args[argc] = dest;
         argc++;
      }

      if (in_arg)
      {
         if (in_backslash)
         {
            in_backslash = false;
            *dest        = *text;
            dest++;
         }
         else if (*text == '\\')
         {
            in_backslash = true;
         }
         else if (*text == cur_quote)
         {
            cur_quote = 0;
         }
         else if ((*text == '\'') || (*text == '"') || (*text == '`'))
         {
            cur_quote = *text;
         }
         else if (cur_quote != 0)
         {
            *dest = *text;
            dest++;
         }
         else if (unc_isspace(*text))
         {
            *dest = 0;
            dest++;
            in_arg = false;
            if (argc == num_args)
            {
               break;
            }
         }
         else
         {
            *dest = *text;
            dest++;
         }
      }
      text++;
   }
   *dest = 0;

   return(argc);
} // Args::SplitLine
