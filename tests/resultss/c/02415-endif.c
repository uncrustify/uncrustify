int do_this
(
   int x,
   int y
)
{

   x++;
   #ifdef ABC
      #ifdef DEF
         x += y;
      #endif
   #endif
   a++;
   #ifdef ABC
      b++;
      #ifdef DEF
         c++;
         #ifdef HIJ
            d++;
         #endif
         e++;
      #endif
      f++;
   #endif
   g++;

   return (x)
}
