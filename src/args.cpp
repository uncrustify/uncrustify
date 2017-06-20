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


const char *Args::Param(const char *token)
{
   size_t idx = 0;

   return(Params(token, idx));
}


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

      if (  arg_len >= token_len
         && (memcmp(token, m_values[idx], token_len) == 0))
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


bool Args::GetUsed(size_t idx)
{
   if (  m_used != nullptr
      && idx > 0
      && idx < m_count)
   {
      return((m_used[idx >> 3] & (1 << (idx & 0x07))) != 0);
   }
   return(false);
}


void Args::SetUsed(size_t idx)
{
   if (  m_used != nullptr
      && idx > 0
      && idx < m_count)
   {
      m_used[idx >> 3] |= (1 << (idx & 0x07));
   }
}


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


size_t Args::SplitLine(char *text, char *args[], size_t num_args)
{
   if (text == nullptr || num_args == 0)
   {
      return(0);
   }

   char   cur_quote    = 0;
   bool   in_backslash = false;
   bool   in_arg       = false;
   size_t argc         = 0;
   char   *dest        = text;

   while (  argc <= num_args // maximal number of arguments not reached yet
         && *text != 0)      // end of string not reached yet
   {
      // Detect the start of an arg
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
         else if (  (*text == '\'')
                 || (*text == '"')
                 || (*text == '`'))
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
               break; // all arguments found, we can stop
            }
         }
         else
         {
            *dest = *text;
            dest++;
         }
      }
      text++; // go on with next character
   }
   *dest = 0;

   return(argc);
} // Args::SplitLine
