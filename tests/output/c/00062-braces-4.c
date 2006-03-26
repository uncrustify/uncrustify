int foo(int bar)
{
#ifndef CONFIG_1
   bar -= 3;
#else
   for (j = 0; j < NR_CPUS; j++)
   {
      if (cpu_online(j))
      {
         bar++;
      }
   }
#endif
   return(0);
}

