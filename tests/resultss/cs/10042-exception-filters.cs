using System;
class Test
{
   void TestExceptionFilter()
   {
      var when = new Object();

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
         int a = (int)when.foo();
      }
      catch (Exception e) when (DateTime.Now.DayOfWeek == DayOfWeek.Saturday)
      {
         string b = ((int)when.prop).ToString();
      }
   }
}
