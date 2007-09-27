class X : Y {
   int foo1;
   #pragma region something
   int foo2 = 2;
  #pragma endregion
  int foo()
  {

   #pragma region something else
   int foo3 = 3;
   #pragma region nested
   int foo4 = 0;
  #pragma endregion
   int foo5 = 0;
  #pragma endregion
  }

}


