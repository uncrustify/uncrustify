// Test: Recursive align_assign with different contexts (enum vs regular)
// Tests that budget resets correctly when entering/exiting nested blocks
// Covers: empty lines, comment lines, and preprocessor lines

class TestClass
{
public:
    // Member assignments (uses assign_skip_cfg)
    VeryLongMemberType m1 = 100;
    // Comment 1
    int m2 = 200;
#ifdef FEATURE_A
    short m3 = 300;
#endif

    // Method with default arguments (tests DEFAULT_ARG alignment)
    void method_with_defaults(VeryLongParamType p1 = 111,
                              // Comment between params
                              int p2 = 222);

    void outer_method()
    {
        // Local var defs (uses assign_skip_cfg via vdas)
        VeryLongLocalType local1 = 1;
        // Comment A
        int local2 = 2;
#ifdef DEBUG
        short local3 = 3;
#endif

        // Nested block - recursion with assign context
        {
            VeryLongNestedType nested1 = 10;
            // Comment B
            short nested2 = 20;
#ifdef NESTED_FEATURE
            int nested3 = 30;
#endif
        }

        // Enum inside method - recursion with enum context (uses enum_skip_cfg)
        enum LocalEnum
        {
            VERY_LONG_ENUM_VALUE_NAME = 1000,
            // Enum comment 1
            SHORT_VAL = 2000,
#ifdef ENUM_FEATURE
            MEDIUM_VALUE = 3000,
#endif
            // Enum comment 2
            TINY = 4000
        };

        // More local vars after enum
        VeryLongType after1 = 111;

        int after2 = 222;
    }

    // Nested struct with enum inside
    struct OuterStruct
    {
        VeryLongMemberType member1 = 100;
#ifdef STRUCT_FEATURE
        int member2 = 200;
#endif
        // Struct comment
        short member3 = 300;

        enum InnerEnum
        {
            LONG_INNER_VALUE = 10;
            // Inner enum comment
            X = 20,
#ifdef INNER_PP
            YY = 30
#endif
        };
    };
};
