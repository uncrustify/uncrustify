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
   size_t m_count;     //! number of command line arguments
   char   **m_values;  //! pointer array to each argument
   UINT8  *m_used;     //! bit array with one flag per argument

public:
   /**
    * Initializes the argument library.
    * Store the values and allocates enough memory for the 'used' flags.
    * This keeps a reference to argv, so don't change it.
    *
    * @param argc  number of command line parameter passed to main()
    * @param argv  pointer array to command line parameters
    */
   Args(int argc, char **argv);

   //! Standard destructor
   ~Args();

   /**
    * Checks to see if an arg w/o a value is present.
    * Scans the args looking for an exact match.
    *
    * "-c" matches "-c", but not "-call" or "-ec"
    *
    * @param token  The token string to match
    *
    * @return true/false -- Whether the argument was present
    */
   bool Present(const char *token);

   /**
    * Calls Args::Params() with index 0
    *
    * @param token  The token string to match
    *
    * @return nullptr or the pointer to the string
    */
   const char *Param(const char *token);

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
    * @param          token  the token string to match
    * @param[in, out] index  search start position, in case that something is
    *                        found, it will get the succeeding position number
    *                        assigned
    *
    * @return nullptr or the pointer to the string
    */
   const char *Params(const char *token, size_t &index);

   /**
    * Marks an argument as being used.
    *
    * @param idx  The index of the argument
    */
   void SetUsed(size_t idx);

   /**
    * Gets whether an argument has been used, by index.
    *
    * @param idx  The index of the argument
    */
   bool GetUsed(size_t idx);

   /**
    * This function retrieves all unused parameters.
    * You must set the index before the first call.
    * Set the index to 1 to skip argv[0].
    *
    * @param idx  Pointer to the index
    *
    * @return nullptr (done) or the pointer to the string
    */
   const char *Unused(size_t &idx);

   /**
    * Takes text and splits it into arguments.
    * args is an array of char pointers that will get populated.
    * num_args is the maximum number of args split off.
    * If there are more than num_args, the remaining text is ignored.
    * Note that text is modified (zeroes are inserted)
    *
    * @param      text      the text to split (modified)
    * @param[out] args      array of pointers to be populated
    * @param      num_args  number of items in input string
    *
    * @return The number of arguments parsed (always <= num_args)
    */
   static size_t SplitLine(char *text, char *args[], size_t num_args);
};


#endif /* ARGS_H_INCLUDED */
