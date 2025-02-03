using System;
class Test
{
   void TestExceptionFilter()
   {
      var whe2 = new Object();

      try
      {
         int i = 0;
      }
      catch (Exception e)
      {
         int j = -1;
      }
      try
      {
         int i = 0;
      }
      catch
      {
         int j = -1;
      }
      try
      {
         int i = 0;
      }
      catch when (DateTime.Now.DayOfWeek == DayOfWeek.Saturday)
      {
         int j = -1;
      }
      try
      {
         int a = (int)whe2.foo();
      }
      catch (Exception e) when (DateTime.Now.DayOfWeek == DayOfWeek.Saturday)
      {
         string b = ((int)whe2.prop).ToString();
      }
   }
}
