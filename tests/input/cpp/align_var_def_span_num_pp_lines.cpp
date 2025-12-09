// Test: align_var_def_span_num_pp_lines in C++ methods
// With span=1 and varying num_pp_lines settings.

class TestClass
{
public:
    void test_method()
    {
        int x;
        unsigned long int y;
#ifdef DEBUG
        double z;
#endif
#ifdef EXTRA
        float w;
#endif
#ifdef MORE
#ifdef NESTED
        long s;
#endif
#endif
    }
};
