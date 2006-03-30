/**
 * @file args.h
 * Parses command line arguments.
 *
 * This differs from the GNU/getopt way in that:
 *  - parameters cannot mixed "-e -f" is not the same as "-ef"
 *  - knowledge of the complete set of parameters is not required
 *  - this means you can handle args in multiple spots
 *  - it is more portible
 *
 * $Id: c_args.c 121 2006-03-27 02:24:45Z bengardner $
 */

#ifndef ARGS_H_INCLUDED
#define ARGS_H_INCLUDED

#include "base_types.h"


/**
 * Initializes the argument library.
 * This keeps a reference to argv, so don't change it.
 *
 * @param argc The argc that was passed to main()
 * @param argv The argv that was passed to main()
 */
void arg_init(int argc, char **argv);


/**
 * Checks to see if an arg w/o a value is present.
 * Just scans the args looking for an exact match.
 *
 * "-c" matches "-c", but not "-call" or "-ec"
 *
 * @param token   The token string to match
 * @return        TRUE/FALSE -- Whether the argument was present
 */
BOOL arg_present(const char *token);


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
const char *arg_param(const char *token);


/**
 * Similiar to arg_param, but can iterate over all matches.
 * Set index to 0 before the first call.
 *
 * @param token   The token string to match
 * @param idx     Pointer to the index that you initialized to 0
 * @return        NULL or the pointer to the string.
 */
const char *arg_params(const char *token, int *idx);


/**
 * Gets whether an argument has been used, by index.
 *
 * @param idx  The index of the argument
 */
BOOL arg_get_used(int idx);


/**
 * Marks an argument as being used.
 *
 * @param idx  The index of the argument
 */
void arg_set_used(int idx);


/**
 * This function retrieves all unused parameters.
 * You must set the index before the first call.
 * Set the index to 1 to skip argv[0].
 *
 * @param idx  Pointer to the index
 * @return     NULL (done) or the pointer to the string
 */
const char *arg_unused(int *idx);


#endif   /* ARGS_H_INCLUDED */

