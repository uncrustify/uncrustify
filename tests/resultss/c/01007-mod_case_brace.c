int foo(int bar)
{
   switch (bar)
   {
   case 0:
      {
         showit(0);
      }
      c++;
      break;

   case 1:
      {
         showit(bar);
         break;
      }

   case 2:
      {
         break;
      }

   case 3:
      {
         int a = bar * 3;
         showit(a);
      }
      c++;
      break;

   case 4:
      {
         foo(bar - 1);
         {
            showit(0);
         }
      }

   case 10:
      {
         switch (gl_bug)
         {
         case 'a':
            {
               gl_foo = true;
               break;
            }

         case 'b':
         case 'c':
            {
               gl_foo = false;
               break;
            }

         default:
            {
               // nothing
               break;
            }
         }
         break;
      }

   default:
      {
         break;
      }
   }
   return(-1);
}
