/**
 * @file punctuators.h
 */

#ifndef PUNCTUATORS_H_INCLUDED
#define PUNCTUATORS_H_INCLUDED

#include "uncrustify_types.h"


struct lookup_entry_t
{
   char              ch;
   char              left_in_group;
   UINT16            next_idx;
   const chunk_tag_t *tag;

   struct comperator
   {
      static char get_char(const lookup_entry_t &v)
      {
         return(v.ch);
      }


      static char get_char(char t)
      {
         return(t);
      }

      template<typename T1, typename T2>
      bool operator()(T1 const &t1, T2 const &t2)
      {
         return(get_char(t1) < get_char(t2));
      }
   };
};


/**
 * Checks if the first max. 6 chars of a given string match a punctuator
 *
 * @param str         string that will be checked, can be shorter than 6 chars
 * @param lang_flags  specifies from which language punctuators will be considered
 *
 * @retval chunk tag of the found punctuator
 * @retval nullptr if nothing found
 */
const chunk_tag_t *find_punctuator(const char *str, int lang_flags);


#endif /* PUNCTUATORS_H_INCLUDED */
