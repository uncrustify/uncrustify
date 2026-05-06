void f(int n, int m)
{
    for (int i = 0; i < n; ++i) {}
    for (int i = 0; i < n; i += 2) {}
    for (int j = 0, k = 0; j < n && k < m; ++j) {}
    for (; n == m;) {}
}
