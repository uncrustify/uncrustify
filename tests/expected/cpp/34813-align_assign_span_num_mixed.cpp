// Test: align_assign_span_num_mixed - Mixed empty, comment, and PP lines

class TestClass
{
public:
void test_method()
{
    VeryLongVariableType var1 = 100;
    int var2                  = 200;
#ifdef FEATURE_A
    // Platform specific
    double var3               = 300.0;

    // Another comment
    short var4                = 400;
#endif
#ifdef FEATURE_B
    // Multiple lines here
    long int var5      = 500;
    // Comment line 1
    // Comment line 2
    // Comment line 3
    unsigned long var6 = 600;


    // Two empty + comment
    char var7  = 700;
#endif

    float var8 = 800;
}
};

class SecondClass
{
public:
void second_method()
{
    char var9 = 'a';


    int var10 = 42;
}

void third_method()
{
    char var11 = 'a';
#if 1
#if 1
#if 1
    int var12 = 42;
#endif
#endif
#endif
}

void fourth_method()
{
    char var13 = 'a';
    // cmt 1
    // cmt 2
    // cmt 3
    // cmt 4
    int var14 = 42;
}
};
