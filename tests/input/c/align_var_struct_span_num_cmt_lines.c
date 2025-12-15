// Test: align_var_struct_span_num_cmt_lines
// With span=1 and varying num_cmt_lines settings.

struct TestStruct
{
    int x; //!< first member
    unsigned long int y; //!< second member
    double z; //!< third member
    //!  continued on next line
    float w; //!< fourth member
    //!  continued
    //!  on two more lines
    long s; //!< fifth member
};
