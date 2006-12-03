void foo(void)
{
   if ((a != 0)
       && (b == 0)
       && (c < 0) && (d > 0))
   {
      printf("hi");
   }

   if (flag1
#ifdef FLAG2
       || flag2
#endif
   )
   {
      printf("yar");
   }

   if ((a != 0)
       && (b == 0)
       && (c < 0))
   {
      printf("hi");
   }

   if ((a != 0)
       &&
       (b == 0)
       &&
       (c < 0))
   {
      printf("hi");
   }

   if (!this->writeOwiFile ()                   // comment1
       || broken () || !saveArchiveData ()      /* comment2 */
       || broken () || !deleteCentralArchive () // comment3
       || broken () || !copyArchivFiles ()      // comment4
       || broken () || !appendToPlanetDb ())    // comment5
   {
      ;
   }
}

