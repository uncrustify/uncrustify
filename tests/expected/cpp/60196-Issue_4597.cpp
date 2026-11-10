class Test
{
Test(uint32_t arg = 16 * 1024 * 1024) {
}

void f(int*p = nullptr, uint32_t size = 4 * 1024, char*q = nullptr);
};

void g(uint32_t a = 2 * 3, int*p = nullptr, int b = 1 * 2 * 3);
