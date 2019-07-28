void f1()
{
   int a;
   int b;
   auto lambda1 = [ &a ](){ return true; };
   auto lambda2 = [ &a = b ](){ return true; };
}
