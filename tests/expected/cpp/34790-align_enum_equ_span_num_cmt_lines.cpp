// Test: align_enum_equ_span_num_cmt_lines - Comment lines between enum values

class TestClass
{
public:
enum TestEnum
{
    VERY_LONG_VALUE_NAME = 100,
    // Single comment
    SHORT_VAL            = 200,
    // Comment 1
    // Comment 2
    MEDIUM_VALUE = 300,
    // Comment 1
    // Comment 2
    // Comment 3
    ANOTHER_LONG_NAME = 400,

    FINAL_VAL = 500
};
};
