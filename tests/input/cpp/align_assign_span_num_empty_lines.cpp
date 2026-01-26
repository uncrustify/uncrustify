// Test: align_assign_span_num_empty_lines - Empty lines between assignments

class TestClass
{
public:
    void test_method()
    {
        VeryLongVariableType var1 = 100;

        int var2 = 200;


        double var3 = 300.0;



        unsigned char var4 = 400;

        long int var5 = 500;
    }
};

// Test with span=1: each group should align independently based on num_empty_lines filter
