void foo(void)
{
   switch(ch)
   {
      case 'a':
         {
            handle_a();
            break;
         }

      case 'b':
         handle_b();
         break;

      case 'c':
      case 'd':
         handle_cd();
         break;

      case 'e':
         {
            handle_a();
         }
         break;

      default:
         handle_default();
         break;
   }
}

