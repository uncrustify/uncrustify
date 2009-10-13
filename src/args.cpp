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
   int len = (argc >> 3) + 1;
   m_used = new UINT8[len];
   if (m_used != NULL)
   {
      memset(m_used, 0, len);
   }
}


Args::~Args()
{
   if (m_used != NULL)
   {
      delete[] m_used;
      m_used = NULL;
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
   int idx;

   if (token != NULL)
   {
      for (idx = 0; idx < m_count; idx++)
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
   int idx = 0;

   return(Params(token, idx));
}


/**
 * Scan for a match
 *
 * @param token   The token string to match
 * @return        NULL or the pointer to the string
 */
const char *Args::Params(const char *token, int& index)
{
   int idx;
   int token_len;
   int arg_len;

   if (token == NULL)
   {
      return(NULL);
   }

   token_len = (int)strlen(token);

   for (idx = index; idx < m_count; idx++)
   {
      arg_len = (int)strlen(m_values[idx]);

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

   return(NULL);
}


/**
 * Gets whether an argument has been used, by index.
 *
 * @param idx  The index of the argument
 */
bool Args::GetUsed(int idx)
{
   if ((m_used != NULL) && (idx >= 0) && (idx < m_count))
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
void Args::SetUsed(int idx)
{
   if ((m_used != NULL) && (idx >= 0) && (idx < m_count))
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
const char *Args::Unused(int& index)
{
   int idx;

   if (m_used == NULL)
   {
      return(NULL);
   }

   for (idx = index; idx < m_count; idx++)
   {
      if (!GetUsed(idx))
      {
         index = idx + 1;
         return(m_values[idx]);
      }
   }
   index = m_count;
   return(NULL);
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
int Args::SplitLine(char *text, char *args[], int num_args)
{
   char cur_quote    = 0;
   bool in_backslash = false;
   bool in_arg       = false;
   int  argc         = 0;
   char *dest        = text;


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
}
