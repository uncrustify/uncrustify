void f(int n)
{
    int a = 0;
    ++a;
    a += 2;

    for (int i = 0; i < n; ++i) {}
    for (int i = 0; i < n; --i) {}
    for (int i = 0; i < n; i++) {}
    for (int i = 0; i < n; i--) {}
    for (int i = 0; i < n; i += 2) {}
    for (int i = 0; i < n; i = i + 1) {}
    for (int j = 0, k = n; j < k; ++j, --k) {}
    for (; a == 0;) {}
}
