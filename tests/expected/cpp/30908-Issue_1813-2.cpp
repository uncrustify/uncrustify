namespace n1 {
namespace n2 {

   void func() {
      another_func([]() {
         return 42;
      });
   }

}
}
