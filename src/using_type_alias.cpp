/**
 * @file using_type_alias.cpp
 * Look after a Type alias
 * https://en.cppreference.com/w/cpp/language/type_alias
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "using_type_alias.h"

#include "chunk_list.h"
#include "keywords.h"


void using_type_alias(chunk_t chunk)
{
   // Issue #3291
   if (using_found)
   {
      if (candidate_found)
      {
         if (assign_found)
         {
            if (type_found)
            {
               // look for a ';' character
               if (chunk.type == CT_WHITESPACE)
               {
                  // do nothing
               }
               else if (chunk.type == CT_SEMICOLON)
               {
                  add_keyword(candidate->text(), CT_TYPE);
                  // reset
                  using_found     = false;
                  candidate_found = false;
                  delete (candidate);
                  candidate    = nullptr;
                  assign_found = false;
                  type_found   = false;
               }
               else
               {
                  // error
                  using_found     = false;
                  candidate_found = false;
                  delete (candidate);
                  candidate    = nullptr;
                  assign_found = false;
                  type_found   = false;
               }
            } // type_found
            else
            {
               // look for a type
               if (chunk.type == CT_WHITESPACE)
               {
                  // do nothing
               }
               else if (chunk.type == CT_TYPE)
               {
                  type_found = true;
                  //the_type   = chunk_dup(&chunk);
               }
               else
               {
                  // error
                  using_found     = false;
                  candidate_found = false;
                  delete (candidate);
                  candidate    = nullptr;
                  assign_found = false;
               }
            } // type_found
         } // assign_found
         else
         {
            // look for a '=' character
            if (chunk.type == CT_WHITESPACE)
            {
               // do nothing
            }
            else if (chunk.type == CT_ASSIGN)
            {
               assign_found = true;
            }
            else
            {
               // error
               using_found     = false;
               candidate_found = false;
               delete (candidate);
               candidate = nullptr;
            }
         } // assign_found
      } // candidate_found
      else
      {
         // look for a candidate
         if (chunk.type == CT_WHITESPACE)
         {
            // do nothing
         }
         else if (chunk.type == CT_WORD)
         {
            candidate_found = true;
            candidate       = chunk_dup(&chunk);
         }
         else
         {
            // error
            using_found = false;
         }
      } // candidate_found
   } // using_found
   else
   {
      // looking for using
      if (chunk.type == CT_USING)
      {
         using_found = true;
      }
   } // using_found
} // using_type_alias
