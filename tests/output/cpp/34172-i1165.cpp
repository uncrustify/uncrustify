#include <functional>

int main()
{
    typedef std::function<void ()> C;
    C callback =
        [] ()
        {
            C f([]()
            {
                int i;
            });
        };
}
