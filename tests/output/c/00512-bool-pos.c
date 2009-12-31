void foo(void)
{
   if ((a != 0) &&
       (b == 0) &&
       (c < 0) &&
       (d > 0))
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

   if (flag1 &&
#ifdef FLAG2
       flag2 &&
#endif
       flag3)
   {
      printf("bo");
   }

   if ((a != 0) &&
       (b == 0) &&
       (c < 0))
   {
      printf("hi");
   }

   if ((a != 0) &&
       (b == 0) &&
       (c < 0))
   {
      printf("hi");
   }

   if (!this->writeOwiFile () ||                // comment1
       broken () ||
       !saveArchiveData () ||                   /* comment2 */
       broken () ||
       !deleteCentralArchive () ||              // comment3
       broken () ||
       !copyArchivFiles () ||                   // comment4
       broken () ||
       !appendToPlanetDb ())                    // comment5
   {
      ;
   }

   foobar(param1,
          param2,
          param3,
          param4);

   foobar2(param1,
           param2,
           param3,
           param4);
}

