#include <vector>

std::vector<int> x;

void f1()
{
   int v = x.empty()
         /**/ ? x.size()
         /**/ : x.size();
}

void f2()
{
   int v = x.empty()
         ? x.size()
         : x.size();
}
