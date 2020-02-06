namespace blah0
{
namespace blah
{
   void func0()
   {
      functionThatTakesALambda( [&]() -> void
      {
         lambdaBody;
      });
   }
}
}
