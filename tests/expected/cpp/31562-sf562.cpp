#include "bar.h"

class Foo : public Bar {
    int foo(int bar) const {
        while (true) {
            baz(&operator[](bar));
        }
    }
};
