// Test: align_assign_decl_func = 1 with span_num_* options
// Tests branches 252 (fcnDefault for DEFAULT_ARG) and 259 (fcnProto for FUNC_PROTO)

class TestClass
{
public:
    // Test CT_ASSIGN_FUNC_PROTO with comments between them (branch 259)
    virtual void f1() = 0;
    // Comment line 1
    virtual void very_long_function_name2() = delete;
    // Comment line 2
    // Comment line 3
    virtual void f3() = default;

    // Test CT_ASSIGN_DEFAULT_ARG with comments between them (branch 252)
    void method1(int p1 = 100,
                 // Comment between params
                 VeryLongTypeName p2 = 200);

    void method2(VeryLongTypeName param1 = 1000,
                 // Comment 1
                 // Comment 2
                 int param2 = 2000,
                 // Comment 3
                 short param3 = 3000);

    // Test CT_ASSIGN_FUNC_PROTO with empty lines (branch 259)
    virtual void short_f() = 0;

    virtual void a_very_long_function_name_here() = delete;


    virtual void tiny() = default;

    // Test CT_ASSIGN_DEFAULT_ARG with empty lines (branch 252)
    void emptyline_method(int p1 = 10,

                          VeryLongTypeName p2 = 20,


                          short p3 = 30);
};
