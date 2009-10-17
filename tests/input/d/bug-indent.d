class C
{
   //--------------| <= (1)
   Axxxxxxxxxxxxxxxx.A createAssignment()
   {
      return(null);
   }
   void func2(){
      foreach (v; container) {
         v.f();
      }
   }

   //                  | <= (2)
   void func3(TemplType !(T) aValue){
   }
}
