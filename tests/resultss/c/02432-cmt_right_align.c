
int foo1(int bar)
{
   if(bar)
   {
      if(b)
      {
         if(a)
         {
            if(r)
            {
               return(r);       /* cool */
            }
            else /* if (r) */
            {
               return(bar);     /* uncool */
            } /* if (r) */
         } /* if (a) */
      } /* if (b) */
   } /* if (bar) */

   return(-1);
} /* foo */

int foo2(int bar)
{
   if(bar)
   {
      if(b)
      {
         if(a)
         {
            if(r)
            {
               return(r);
            }
            else
            {
               return(bar);
            } /* if (r) */
         } /* if (a) */
      } /* if (b) */
   } /* if (bar) */

   return(-1);
} /* foo */

