/**
 * @file change_int_types.cpp
 *
 * @author  Alex Henrie
 * @license GPL v2+
 */

#include "change_int_types.h"

#include "chunk.h"
#include "uncrustify.h"


using namespace uncrustify;


static bool add_or_remove_int_keyword(Chunk *pc, iarf_e action)
{
   Chunk *next = pc->GetNextNcNnl();

   // Find the next token that is not a storage keyword
   while (  strcmp(next->Text(), "const") == 0
         || strcmp(next->Text(), "static") == 0
         || strcmp(next->Text(), "volatile") == 0)
   {
      next = next->GetNextNcNnl();
   }

   // Skip over all but the last size/sign qualifier before the 'int' keyword
   if (  strcmp(next->Text(), "long") == 0
      || strcmp(next->Text(), "short") == 0
      || strcmp(next->Text(), "signed") == 0
      || strcmp(next->Text(), "unsigned") == 0)
   {
      return(false);
   }

   if (strcmp(next->Text(), "int") == 0)
   {
      if (action == IARF_REMOVE)
      {
         Chunk::Delete(next);
      }
   }
   else if (  strcmp(next->Text(), "char") != 0
           && strcmp(next->Text(), "double") != 0)
   {
      if (  action == IARF_ADD
         || action == IARF_FORCE)
      {
         Chunk *int_keyword = pc->CopyAndAddAfter(pc);
         int_keyword->str = "int";
      }
   }
   return(true);
}


void change_int_types()
{
   LOG_FUNC_ENTRY();

   bool found_int = false;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      // Skip any 'int' keyword that has already been processed, as well as any qualifiers after it
      if (found_int)
      {
         if (  strcmp(pc->Text(), "int") != 0
            && strcmp(pc->Text(), "const") != 0
            && strcmp(pc->Text(), "long") != 0
            && strcmp(pc->Text(), "short") != 0
            && strcmp(pc->Text(), "signed") != 0
            && strcmp(pc->Text(), "static") != 0
            && strcmp(pc->Text(), "unsigned") != 0
            && strcmp(pc->Text(), "volatile") != 0)
         {
            found_int = false;
         }
         continue;
      }

      if (strcmp(pc->Text(), "short") == 0)
      {
         found_int = add_or_remove_int_keyword(pc, options::mod_short_int());
      }
      else if (strcmp(pc->Text(), "long") == 0)
      {
         found_int = add_or_remove_int_keyword(pc, options::mod_long_int());
      }
      else if (strcmp(pc->Text(), "signed") == 0)
      {
         found_int = add_or_remove_int_keyword(pc, options::mod_signed_int());
      }
      else if (strcmp(pc->Text(), "unsigned") == 0)
      {
         found_int = add_or_remove_int_keyword(pc, options::mod_unsigned_int());
      }
      else if (strcmp(pc->Text(), "int") == 0)
      {
         found_int = true;
      }
   }
} // change_int_types
