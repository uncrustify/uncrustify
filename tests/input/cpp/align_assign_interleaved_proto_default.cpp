// Test: Mixed FUNC_PROTO and DEFAULT_ARG with decl_func=1
// With span=1 and types intercalated, alignment won't occur because
// intermediate code lines count in the span (expected behavior).
// With span=3+, alignment should work.

class MixedTest
{
public:
    // Interleaved FUNC_PROTO and DEFAULT_ARG
    virtual void long_proto_name1() = 0;
    // Comment 1
    void method_with_default(int p = 100);
    // Comment 2
    virtual void f2() = delete;
    // Comment 3
    void another_method(VeryLongType x = 200);
    // Comment 4
    virtual void long_proto_name3() = default;

    // Same but with empty lines
    virtual void empty_proto1() = 0;

    void a_method(int p = 10);

    virtual void empty_long_proto_name2() = delete;

    void empty_another(VeryLongType x = 20);

    virtual void empty_proto3() = default;
};
