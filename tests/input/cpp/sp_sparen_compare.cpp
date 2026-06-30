void f(int n, int m)
{
    bool b = n < m;

    while (n < m) { ++n; }
    do { ++n; } while (n != m);
    if (n == 0) { ; }
    if (n > 1) { ; } else if (n < 0) { ; }
    switch (n < 0 ? -n : n) { default: break; }

    for (int i = 0; i < n; ++i) {}
}
