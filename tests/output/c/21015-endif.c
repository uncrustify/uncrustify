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

   return (x)
}
