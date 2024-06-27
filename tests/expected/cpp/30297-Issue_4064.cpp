#include <iostream>

int main() {
        int a, b;
        std::pair<int&, int&> p(a, b);

        void foo(int&, int&);
}
