/**
 * trailing comments are added at 8 newlines in this test.
 *
 *
 */
void short_function(void)
{
   /* this is a 'short' function, so no added comment */
}

void long_function(void)
{
   /* this is a 'long' function, so a comment is added */
   switch (some_int_value())
   {
   case 0:
      handle_zero();
      break;

   case 50:
      handle_fifty();
      break;

   case 127:
      handle_another_value();
      break;

   default:
      boy_do_i_lack_imagination();
      break;
   }

   /* call one last function... */
   one_last_func_call();
}

