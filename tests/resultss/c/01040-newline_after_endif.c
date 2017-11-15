int main
(
)
{
   #ifdef useJPLvelocity
      for(i = 0; i < x; i++)
         y++;
   #endif

   return (0);
} /* main */

int main
(
)
{
   if(y < 3)
      y++;

   #ifdef ABC
      if(y < 3)
         y++ // comment
   #endif

   if(y < 3)
      y++;

   y++;

   return (0);
} /* main */

int main
(
)
{
   #ifdef ABC
      if(j < y)
      {
         if(j < x)
         {
            j++;
            #ifdef XYZ
               if(j < x)
                  j++;
            #endif
         }
      }
   #endif

   return (0);
} /* main */

