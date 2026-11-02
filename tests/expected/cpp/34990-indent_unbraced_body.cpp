// Test for the indent_unbraced_body option: the body of an unbraced
// if/else/for/while/do statement is indented by a fixed step (2) instead
// of indent_columns (3).

void unbraced_bodies(int a, int b)
{
   if (!a)
     return;

   if (a)
     if (b)
       deep();

   for (int i = 0; i < 10; i++)
     work(i);

   while (running)
     step();

   do
     once();
   while (a > 0);

   if (a)
     one();
   else
     two();
}
