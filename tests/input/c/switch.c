
int foo(int op)
{
   switch (op)
   {
   case 1: {
         do_something();
         return 0;
      }

   case 2: 
      do_something_else();
      return 1;

   case 3:
   case 4:
      /* don't do anything */
      break;

   case 5:
      return 3;

   default:
      break;
   }

   return -1;
}

