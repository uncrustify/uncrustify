// Test: align_var_class_span_num_cmt_lines
// With span=1 and and varying num_cmt_lines settings.

class TestClass
{
public:
    int x; //!< first member
    unsigned long int y; //!< second member
    //!  continued on next line
    double z; //!< third member
    //!  continued
    //!  on two more lines
    float w; //!< fourth member
    //!  continued
    //!  on three
    //!  more lines
    short s; //!< fifth member
};
