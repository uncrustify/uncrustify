
#include "chunk_stack.h"
#include <malloc.h>


void cs_reset(void)
{
   cpd.cs_len = 0;
}

void cs_push(chunk_t *pc)
{
   chunk_stack_t *tmp;

   if (cpd.cs_len >= cpd.cs_size)
   {
      /* double the size */
      if (cpd.cs != NULL)
      {
         tmp = realloc(cpd.cs, (cpd.cs_size + 256) * sizeof(chunk_stack_t));
      }
      else
      {
         tmp = malloc(256 * sizeof(chunk_stack_t));
      }
      if (tmp != NULL)
      {
         cpd.cs       = tmp;
         cpd.cs_size += 256;
      }
   }

   if (cpd.cs_len < cpd.cs_size)
   {
      cpd.cs[cpd.cs_len++].pc = pc;
   }
}

chunk_t *cs_pop(void)
{
   if (cpd.cs_len > 0)
   {
      cpd.cs_len--;
      return(cpd.cs[cpd.cs_len].pc);
   }
   return(NULL);
}

int cs_len(void)
{
   return(cpd.cs_len);
}

