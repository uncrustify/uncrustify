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


static bool is_storage_keyword(const Chunk *pc)
{
   return(  strcmp(pc->Text(), "auto") == 0
         || strcmp(pc->Text(), "const") == 0
         || strcmp(pc->Text(), "extern") == 0
         || strcmp(pc->Text(), "mutable") == 0
         || strcmp(pc->Text(), "register") == 0
         || strcmp(pc->Text(), "static") == 0
         || strcmp(pc->Text(), "thread_local") == 0
         || strcmp(pc->Text(), "typedef") == 0
         || strcmp(pc->Text(), "volatile") == 0
         || strcmp(pc->Text(), "_Atomic") == 0
         || strcmp(pc->Text(), "_Thread_local") == 0);
}


static bool is_non_integer(const Chunk *pc)
{
   return(  strcmp(pc->Text(), "char") == 0
         || strcmp(pc->Text(), "double") == 0);
}


static bool find_non_storage_siblings(const Chunk *pc, Chunk * &prev, Chunk * &next)
{
   prev = pc->GetPrevNcNnl();
   next = pc->GetNextNcNnl();

   // Find the last token that was not a storage keyword
   while (is_storage_keyword(prev))
   {
      prev = prev->GetPrevNcNnl();
   }

   // Return false if the last token indicates that this is not an integer type
   if (is_non_integer(prev))
   {
      return(false);
   }

   // Find the next token that is not a storage keyword
   while (is_storage_keyword(next))
   {
      next = next->GetNextNcNnl();
   }

   // Return false if the next token indicates that this is not an integer type
   if (is_non_integer(next))
   {
      return(false);
   }
   // Return true if this is indeed an integer type
   return(true);
}


static void add_or_remove_int_keyword(Chunk *pc, Chunk *sibling, iarf_e action, E_Direction dir, Chunk * &int_keyword)
{
   if (strcmp(sibling->Text(), "int") == 0)
   {
      if (action == IARF_REMOVE)
      {
         if (sibling == int_keyword)
         {
            int_keyword = Chunk::NullChunkPtr;
         }
         Chunk::Delete(sibling);
      }
      else if (  int_keyword->IsNotNullChunk()
              && int_keyword != sibling)
      {
         // We added an int keyword, but now we see that there already was one.
         // Keep one or the other but not both.
         if (options::mod_int_prefer_int_on_left())
         {
            Chunk::Delete(sibling);
         }
         else
         {
            Chunk::Delete(int_keyword);
            int_keyword = sibling;
         }
      }
      else
      {
         int_keyword = sibling;
      }
   }
   else
   {
      if (  action == IARF_ADD
         || action == IARF_FORCE)
      {
         if (int_keyword->IsNotNullChunk())
         {
            // There was already an int keyword. Either keep it and don't add a
            // new one or delete it to make way for the new one.
            if (options::mod_int_prefer_int_on_left())
            {
               return;
            }
            else
            {
               Chunk::Delete(int_keyword);
            }
         }

         if (dir == E_Direction::BACKWARD)
         {
            int_keyword = pc->CopyAndAddBefore(pc);
         }
         else
         {
            int_keyword = pc->CopyAndAddAfter(pc);
         }
         int_keyword->Str() = "int";
      }
   }
} // add_or_remove_int_keyword


void change_int_types()
{
   LOG_FUNC_ENTRY();

   Chunk *prev;
   Chunk *next;
   Chunk *int_keyword = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (strcmp(pc->Text(), "short") == 0)
      {
         if (find_non_storage_siblings(pc, prev, next))
         {
            add_or_remove_int_keyword(pc, prev, options::mod_int_short(), E_Direction::BACKWARD, int_keyword);
            add_or_remove_int_keyword(pc, next, options::mod_short_int(), E_Direction::FORWARD, int_keyword);
         }
      }
      else if (strcmp(pc->Text(), "long") == 0)
      {
         if (find_non_storage_siblings(pc, prev, next))
         {
            add_or_remove_int_keyword(pc, prev, options::mod_int_long(), E_Direction::BACKWARD, int_keyword);
            add_or_remove_int_keyword(pc, next, options::mod_long_int(), E_Direction::FORWARD, int_keyword);
         }
      }
      else if (strcmp(pc->Text(), "signed") == 0)
      {
         if (find_non_storage_siblings(pc, prev, next))
         {
            add_or_remove_int_keyword(pc, prev, options::mod_int_signed(), E_Direction::BACKWARD, int_keyword);
            add_or_remove_int_keyword(pc, next, options::mod_signed_int(), E_Direction::FORWARD, int_keyword);
         }
      }
      else if (strcmp(pc->Text(), "unsigned") == 0)
      {
         if (find_non_storage_siblings(pc, prev, next))
         {
            add_or_remove_int_keyword(pc, prev, options::mod_int_unsigned(), E_Direction::BACKWARD, int_keyword);
            add_or_remove_int_keyword(pc, next, options::mod_unsigned_int(), E_Direction::FORWARD, int_keyword);
         }
      }
      else if (  strcmp(pc->Text(), "int") != 0
              && !is_storage_keyword(pc))
      {
         int_keyword = Chunk::NullChunkPtr; // We are no longer in a variable declaration
      }
   }
} // change_int_types
