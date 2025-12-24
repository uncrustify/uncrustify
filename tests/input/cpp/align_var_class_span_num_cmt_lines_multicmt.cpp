// Test: align_var_class_span_num_cmt_lines
// With span=1 and and varying num_cmt_lines settings.

class TestClass
{
public:
    int x; //!< first member
    unsigned long int y;
     //!< second member continued on next line
    double z; /* zzz
              continued
              on two more lines */
    int z2; /* zzz
             * continued on three
             * more lines
             */
    long int z3; //!< third member
    //!  continued
    //!  on two more lines
    double z4; /* zzz */
               /* continued */
               /* on two more lines */
    float w; //!< fourth member
    //!  continued
    //!  on three
    //!  more lines
    char s; //!< fifth member
};
