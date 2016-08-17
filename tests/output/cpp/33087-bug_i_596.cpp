#include "child.h"
int main(int argc, char*argv[]) {
    (void)argc;
    (void)argv;
    Child child;
    for (auto &attribute : *child.GetAttributes()) {
        std::cout << attribute << std::endl;
    }
    return 0;
}
