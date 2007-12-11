
void f()
{
   if (0)
   {
#pragma omp atomic
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

