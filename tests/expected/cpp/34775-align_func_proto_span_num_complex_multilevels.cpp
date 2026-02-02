// Test: align_func_proto_span_num_complex - Multi-level alignment with nested structures
// Tests various numbers of comment lines (1, 2, 3, 4) at each level

// Global level (level=0, brace_level=0)
VeryLongGlobalType global_func_one();
// Single comment
int                global_func_two();
// Comment 1
// Comment 2
short global_func_three();
// Comment 1
// Comment 2
// Comment 3
double global_func_four();
// Comment 1
// Comment 2
// Comment 3
// Comment 4
char global_func_five();

namespace OuterNamespace {  // level=1, brace_level=0
    LongNamespaceType ns_func_one();
    // Single comment
    short             ns_func_two();
    // Comment 1
    // Comment 2
    unsigned int ns_func_three();
    // Comment 1
    // Comment 2
    // Comment 3
    float ns_func_four();
    // Comment 1
    // Comment 2
    // Comment 3
    // Comment 4
    long int ns_func_five();

    class OuterClass {  // level=2, brace_level=1
        public:
        VeryLongMethodType outer_method_one();
        // Single comment
        int                outer_method_two();
        // Comment 1
        // Comment 2
        unsigned short outer_method_three();
        // Comment 1
        // Comment 2
        // Comment 3
        double outer_method_four();
        // Comment 1
        // Comment 2
        // Comment 3
        // Comment 4
        long outer_method_five();

        class InnerClass {  // level=3, brace_level=2
            public:
            LongInnerType inner_method_one();
            // Single comment
            char          inner_method_two();
            // Comment 1
            // Comment 2
            unsigned char inner_method_three();
            // Comment 1
            // Comment 2
            // Comment 3
            short inner_method_four();
            // Comment 1
            // Comment 2
            // Comment 3
            // Comment 4
            int inner_method_five();

            struct DeepStruct {  // level=4, brace_level=3
                VeryLongStructType struct_method_one();
                // Single comment
                int                struct_method_two();
                // Comment 1
                // Comment 2
                unsigned int struct_method_three();
                // Comment 1
                // Comment 2
                // Comment 3
                long struct_method_four();
                // Comment 1
                // Comment 2
                // Comment 3
                // Comment 4
                short struct_method_five();
            };
        };

        // Back to level=2, brace_level=1
        float outer_method_six();
    };

    // Back to level=1, brace_level=0
    double ns_func_six();
}

// Back to global
long int global_func_six();
