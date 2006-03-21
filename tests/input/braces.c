
void foo(void)
{
   int a = 0;
   while (a < 3)
   {
      a++;
   }

   while (b < a)
      b++;

   do
   {
      a--;
   } while (a > 0);
   
   do
      a--;
   while (a > 0);

   for (a = 0; a < 10; a++)
   {
      printf("a=%d\n", a);
   }

   if (a == 10)
   {
      printf("a looks good\n");
   }
   else
   {
      printf("not so good\n");
   }

   if (state == ST_RUN)
   {
      if ((foo < bar) &&
          (bar > foo2))
      {
         if (a < 5)
         {
            a *= a;
         }
		 else if (b != 0)
             a /= b;
		 else
			 a += b;
      }
   }
}

