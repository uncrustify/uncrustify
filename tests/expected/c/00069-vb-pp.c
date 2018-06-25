
void f()
{
   if (0)
#pragma omp atomic
   {
      i++;
   }
}


void f()
{
   if (0)
#if foo
   {
      i++;
   }
#else
   {
      i += 2;
   }
#endif
}

void f()
{
   while (108)
   {
      if (42)
#pragma omp critical
      { }
      if (23)
#pragma omp critical
      {
         ++i;
      }
      while (16)
      {
      }
      int i = 15;
      if (8)
#pragma omp atomic
      {
         i += 4;
      }
   }
}

