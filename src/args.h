/**
 * @file args.h
 * Parses command line arguments.
 *
 * This differs from the GNU/getopt way in that:
 *  - parameters cannot mixed "-e -f" is not the same as "-ef"
 *  - knowledge of the complete set of parameters is not required
 *  - this means you can handle args in multiple spots
 *  - it is more portable
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef ARGS_H_INCLUDED
#define ARGS_H_INCLUDED

#include "base_types.h"

class Args
{
protected:
   int   m_count;
   char  **m_values;
   UINT8 *m_used;      /* array of bits */

public:

   /**
    * Initializes the argument library.
    * This keeps a reference to argv, so don't change it.
    *
    * @param argc The argc that was passed to main()
    * @param argv The argv that was passed to main()
    */
   Args(int argc, char **argv);

   /** Standard destructor */
   ~Args();

   /**
    * Checks to see if an arg w/o a value is present.
    * Just scans the args looking for an exact match.
    *
    * "-c" matches "-c", but not "-call" or "-ec"
    *
    * @param token   The token string to match
    * @return        true/false -- Whether the argument was present
    */
   bool Present(const char *token);

   /**
    * Check for an arg with a value.
    * Returns only the first match.
    *
    * Assuming the token "-c"...
    *   "-call" returns "all"
    *   "-c=all" returns "all"
    *   "-c", "all" returns "all"
    *   "-c=", "all" returns ""
    *
    * @param token   The token string to match
    * @return        NULL or the pointer to the string
    */
   const char *Param(const char *token);

   /**
    * Similiar to arg_param, but can iterate over all matches.
    * Set index to 0 before the first call.
    *
    * @param token   The token string to match
    * @param idx     Pointer to the index that you initialized to 0
    * @return        NULL or the pointer to the string.
    */
   const char *Params(const char *token, int& index);

   /**
    * Marks an argument as being used.
    *
    * @param idx  The index of the argument
    */
   void SetUsed(int idx);

   /**
    * Gets whether an argument has been used, by index.
    *
    * @param idx  The index of the argument
    */
   bool GetUsed(int idx);

   /**
    * This function retrieves all unused parameters.
    * You must set the index before the first call.
    * Set the index to 1 to skip argv[0].
    *
    * @param idx  Pointer to the index
    * @return     NULL (done) or the pointer to the string
    */
   const char *Unused(int& idx);

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
   static int SplitLine(char *text, char *args[], int num_args);
};

#endif   /* ARGS_H_INCLUDED */
