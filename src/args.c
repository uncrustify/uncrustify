/**
 * @file args.c
 * Parses command line arguments.
 *
 * $Id: c_args.c 121 2006-03-27 02:24:45Z bengardner $
 */


#include "args.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <malloc.h>


struct args_data
{
   int  count;
   char **values;
   BOOL *used;      /* lazy - could do an array of bits */
};

static struct args_data ad;


/**
 * Store the values and allocate enough memory for the 'used' flags.
 *
 * @param argc The argc that was passed to main()
 * @param argv The argv that was passed to main()
 */
void arg_init(int argc, char **argv)
{
   ad.count  = argc;
   ad.values = argv;
   ad.used   = calloc(1, argc);
}


/**
 * Check for an exact match
 *
 * @param token   The token string to match
 * @return        TRUE/FALSE -- Whether the argument was present
 */
BOOL arg_present(const char *token)
{
   int idx;

   if (token != NULL)
   {
      for (idx = 0; idx < ad.count; idx++)
      {
         if (strcmp(token, ad.values[idx]) == 0)
         {
            if (ad.used != NULL)
            {
               ad.used[idx] = TRUE;
            }
            return(TRUE);
         }
      }
   }

   return(FALSE);
}



/**
 * Just call arg_params() with an index of 0.
 *
 * @param token   The token string to match
 * @return        NULL or the pointer to the string
 */
const char *arg_param(const char *token)
{
   int idx = 0;

   return(arg_params(token, &idx));
}


/**
 * Scan for a match
 *
 * @param token   The token string to match
 * @return        NULL or the pointer to the string
 */
const char *arg_params(const char *token, int *index)
{
   int idx;
   int token_len;
   int arg_len;

   if (token == NULL)
   {
      return(NULL);
   }

   token_len = strlen(token);

   for (idx = index ? *index : 0; idx < ad.count; idx++)
   {
      arg_len = strlen(ad.values[idx]);

      if ((arg_len >= token_len) &&
          (memcmp(token, ad.values[idx], token_len) == 0))
      {
         if (ad.used != NULL)
         {
            ad.used[idx] = TRUE;
         }
         if (arg_len > token_len)
         {
            if (ad.values[idx][token_len] == '=')
            {
               token_len++;
            }
            if (index != NULL)
            {
               *index = idx + 1;
            }
            return(&ad.values[idx][token_len]);
         }
         idx++;
         if (index != NULL)
         {
            *index = idx + 1;
         }
         if (idx < ad.count)
         {
            if (ad.used != NULL)
            {
               ad.used[idx] = TRUE;
            }
            return(ad.values[idx]);
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
BOOL arg_get_used(int idx)
{
   if ((ad.used != NULL) && (ad.count > idx))
   {
      return(ad.used[idx]);
   }
   return(FALSE);
}


/**
 * Marks an argument as being used.
 *
 * @param idx  The index of the argument
 */
void arg_set_used(int idx)
{
   if ((ad.used != NULL) && (ad.count > idx))
   {
      ad.used[idx] = TRUE;
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
const char *arg_unused(int *idx)
{
   int index;

   if ((idx == NULL) || (ad.used == NULL))
   {
      return(NULL);
   }

   for (index = *idx; index < ad.count; index++)
   {
      if (!ad.used[index])
      {
         *idx = index + 1;
         return(ad.values[index]);
      }
   }
   *idx = ad.count;
   return(NULL);
}

