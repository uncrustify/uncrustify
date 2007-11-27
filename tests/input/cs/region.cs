class X : Y {
   int foo1;
   #region something
   int foo2 = 2;
  #endregion
  int foo()
  {

   #region something else
   int foo3 = 3;
   #region nested
   int foo4 = 0;
  #endregion
   int foo5 = 0;
  #endregion
  }

}


