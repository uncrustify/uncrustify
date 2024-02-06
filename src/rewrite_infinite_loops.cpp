/**
 * @file rewrite_infinite_loops.cpp
 *
 * @author  Alex Henrie
 * @license GPL v2+
 */

#include "rewrite_infinite_loops.h"

#include "chunk.h"
#include "newlines/add.h"
#include "uncrustify.h"

using namespace uncrustify;


static bool for_needs_rewrite(Chunk *pc, E_Token desired_type)
{
   // The 'for' statement needs to be rewritten if `for(;;)` is not the
   // preferred syntax for infinite loops and this 'for' is an infinite loop
   // with no extra tokens (such as inline comments).

   if (desired_type == CT_FOR)
   {
      return(false);
   }
   pc = pc->GetNext();

   if (!pc->Is(CT_SPAREN_OPEN))
   {
      return(false);
   }
   pc = pc->GetNext();

   if (!pc->Is(CT_SEMICOLON))
   {
      return(false);
   }
   pc = pc->GetNext();

   if (!pc->Is(CT_SEMICOLON))
   {
      return(false);
   }
   pc = pc->GetNext();

   if (!pc->Is(CT_SPAREN_CLOSE))
   {
      return(false);
   }
   return(true);
}


static bool while_needs_rewrite(Chunk *keyword, E_Token desired_type, const char *desired_condition)
{
   // The 'while' statement needs to be rewritten if it has only the tokens that
   // are strictly necessary (keyword, condition, two parentheses, and semicolon
   // if do-while) and either the keyword or the condition needs to be changed.

   Chunk *oparen    = keyword->GetNext();
   Chunk *condition = oparen->GetNext();
   Chunk *cparen    = condition->GetNext();

   if (!oparen->Is(CT_SPAREN_OPEN))
   {
      return(false);
   }

   if (  strcmp(condition->Text(), "true") != 0
      && strcmp(condition->Text(), "1") != 0)
   {
      return(false);
   }

   if (!cparen->Is(CT_SPAREN_CLOSE))
   {
      return(false);
   }

   if (keyword->Is(CT_WHILE_OF_DO))
   {
      Chunk *semicolon = cparen->GetNext();

      if (!semicolon->Is(CT_SEMICOLON))
      {
         return(false);
      }
   }

   if (!keyword->Is(desired_type))
   {
      return(true);
   }

   if (  strcmp(condition->Text(), "true") == 0
      && strcmp(desired_condition, "true") != 0)
   {
      return(true);
   }

   if (  strcmp(condition->Text(), "1") == 0
      && strcmp(desired_condition, "1") != 0)
   {
      return(true);
   }
   return(false);
} // while_needs_rewrite


void rewrite_loop_keyword(Chunk *keyword, E_Token new_type)
{
   keyword->SetType(new_type);

   switch (new_type)
   {
   case CT_DO:
      keyword->SetOrigColEnd(keyword->GetOrigColEnd() + strlen("do") - keyword->Len());
      keyword->Str() = "do";
      break;

   case CT_WHILE:
   case CT_WHILE_OF_DO:
      keyword->SetOrigColEnd(keyword->GetOrigColEnd() + strlen("while") - keyword->Len());
      keyword->Str() = "while";
      break;

   case CT_FOR:
      keyword->SetOrigColEnd(keyword->GetOrigColEnd() + strlen("for") - keyword->Len());
      keyword->Str() = "for";
      break;

   default:
      break;
   }
}


static void move_one_token(Chunk * &source, Chunk * &destination, E_Token parent_type)
{
   Chunk *next_source = source->GetNext();

   // Place the source token immediately after the destination token, without
   // any whitespace.

   source->MoveAfter(destination);
   source->SetColumn(destination->GetColumn() + destination->Len());
   source->SetOrigCol(destination->GetOrigCol() + destination->Len());
   source->SetOrigColEnd(source->GetOrigColEnd() + source->Len());
   source->SetOrigPrevSp(0);
   source->SetParentType(parent_type);

   destination = source;
   source      = next_source;
}


static void rewrite_loop_condition(Chunk * &source, Chunk * &destination,
                                   E_Token desired_type, const char *desired_condition)
{
   // Move the opening parenthesis
   move_one_token(source, destination, desired_type);

   // Move the condition
   if (desired_type == CT_FOR)
   {
      source->SetType(CT_SEMICOLON);
      source->SetParentType(CT_FOR);
      source->Str() = ";";
      move_one_token(source, destination, desired_type);
      destination = (destination)->CopyAndAddAfter(destination);
   }
   else
   {
      source->SetType(CT_WORD);
      source->Str() = desired_condition;
      move_one_token(source, destination, desired_type);
   }

   // If converting a 'for' to a 'while', delete the second semicolon
   if (source->Is(CT_SEMICOLON))
   {
      Chunk *next_source = source->GetNext();
      Chunk::Delete(source);
      source = next_source;
   }
   // Move the closing parenthesis
   move_one_token(source, destination, desired_type);
}


void rewrite_loop_in_place(Chunk *keyword, E_Token desired_type, const char *desired_condition)
{
   Chunk *top    = keyword->GetNext();
   Chunk *bottom = keyword;

   rewrite_loop_keyword(keyword, desired_type);
   rewrite_loop_condition(top, bottom, desired_type, desired_condition);
}


static Chunk *find_start_brace(Chunk *pc)
{
   while (!pc->IsBraceOpen())
   {
      pc = pc->GetNextNcNnl();
   }
   return(pc);
}


void rewrite_infinite_loops()
{
   LOG_FUNC_ENTRY();

   E_Token    desired_type;
   const char *desired_condition;

   switch (options::mod_infinite_loop())
   {
   case 1: // for(;;)
      desired_type      = CT_FOR;
      desired_condition = nullptr;
      break;

   case 2: // while(true)
      desired_type      = CT_WHILE;
      desired_condition = "true";
      break;

   case 3: // do...while(true)
      desired_type      = CT_WHILE_OF_DO;
      desired_condition = "true";
      break;

   case 4: // while(1)
      desired_type      = CT_WHILE;
      desired_condition = "1";
      break;

   case 5: // do...while(1)
      desired_type      = CT_WHILE_OF_DO;
      desired_condition = "1";
      break;

   default:
      return;
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->Is(CT_DO))
      {
         Chunk *start_brace   = find_start_brace(pc);
         Chunk *end_brace     = start_brace->GetClosingParen();
         Chunk *while_keyword = end_brace->GetNextNcNnl();

         if (  !while_keyword->Is(CT_WHILE_OF_DO)
            || !while_needs_rewrite(while_keyword, desired_type, desired_condition))
         {
            continue;
         }

         if (desired_type == CT_WHILE_OF_DO)
         {
            // Change the loop condition
            rewrite_loop_in_place(while_keyword, desired_type, desired_condition);

            // Update the braces' parent types
            start_brace->SetParentType(CT_DO);
            end_brace->SetParentType(CT_DO);
         }
         else
         {
            Chunk *top    = pc;
            Chunk *bottom = while_keyword->GetNext();

            // Change the 'do' at the top of the loop to a 'for' or a 'while'
            rewrite_loop_keyword(top, desired_type);

            // Delete the 'while' at the bottom of the loop
            Chunk::Delete(while_keyword);

            // Move the rest of the tokens from the bottom to the top
            rewrite_loop_condition(bottom, top, desired_type, desired_condition);

            // Delete the final semicolon
            Chunk::Delete(bottom);

            // Update the braces' parent types
            start_brace->SetParentType(desired_type);
            end_brace->SetParentType(desired_type);
         }
      }
      else if (  (  pc->Is(CT_WHILE)
                 && while_needs_rewrite(pc, desired_type, desired_condition))
              || (  pc->Is(CT_FOR)
                 && for_needs_rewrite(pc, desired_type)))
      {
         Chunk *start_brace = find_start_brace(pc);
         Chunk *end_brace   = start_brace->GetClosingParen();

         if (desired_type == CT_WHILE_OF_DO)
         {
            Chunk *top    = pc;
            Chunk *bottom = end_brace;

            if (bottom->Is(CT_VBRACE_CLOSE))
            {
               // Insert a new line before the new 'while' keyword
               newline_add_before(bottom);
            }
            // Add a 'while' at the bottom of the loop
            bottom = top->CopyAndAddAfter(bottom);
            rewrite_loop_keyword(bottom, CT_WHILE_OF_DO);

            // Change the 'while' at the top of the loop to a 'do'
            rewrite_loop_keyword(top, CT_DO);
            top = top->GetNext();

            // Move the tokens from the top to the bottom
            rewrite_loop_condition(top, bottom, desired_type, desired_condition);

            // Add the final semicolon
            bottom = bottom->CopyAndAddAfter(bottom);
            bottom->SetType(CT_SEMICOLON);
            bottom->Str() = ";";

            // Update the braces' parent types
            start_brace->SetParentType(CT_DO);
            end_brace->SetParentType(CT_DO);
         }
         else
         {
            // Change 'for' to 'while' or vice-versa
            rewrite_loop_in_place(pc, desired_type, desired_condition);

            // Update the braces' parent types
            start_brace->SetParentType(desired_type);
            end_brace->SetParentType(desired_type);
         }
      }
   }
} // rewrite_infinite_loops
