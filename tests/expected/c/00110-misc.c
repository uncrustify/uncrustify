/********************/
/* Before Unrustify */
/********************/

#define MACRO(cond, action)    if ((cond)) (action)

void hurz(murks)
{
   if (murks = 4)              // This comment belongs to (murks = 4)
   {
      schrott();               // And this to schrott()
   }
#ifdef SCHNIEPEL
   else if (murks = 6)         // This comment belongs to (murks = 6)
                               // I had to write more comment than one line
                               // so I inserted some comment only blocks
   {
      schniepel();
   }
#endif // SCHNIEPEL
   else
   {
      flursen();
   }

   if (murks = 4)              // This comment belongs to (murks = 4)
#ifdef FOO
   {
      foo();
   }
#else
   {
      bar();
   }
#endif
   if (murks = 4)                 // This comment belongs to (murks = 4)
   {
      schrott();                  // And this to schrott()
   }
   return;
} // hurz()

