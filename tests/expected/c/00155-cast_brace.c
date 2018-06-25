//
void disappearing_semicolon(void)
{
   r = (recordtypecast){
      a, b, c
   };                          //<--
   p = Table_put(t, a, &r);
}
