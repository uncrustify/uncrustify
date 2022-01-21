#include <iostream>

bool
foo()
{
    const int i = 3;

    if ( i == 2 || i == 3 || i == 5 ) {
        std::cerr << "Very small prime!\n";
    }

    const auto isSmallPrime = i == 2 || i == 3 || i == 5 || i == 7 || i == 11;

    return isSmallPrime || i == 13 || i == 17 || i == 19;
}
