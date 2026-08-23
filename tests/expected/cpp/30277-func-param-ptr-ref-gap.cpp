void reference_gaps(int                     value,
                    long                   &reference,
                    unsigned              &&rvalue_reference,
                    unsigned long long    &&other_rvalue_reference)
{
}

void reference_widths(int            value,
                      int           &lvalue_reference,
                      int          &&rvalue_reference,
                      long long    &&other_rvalue_reference)
{
}

void no_reference_gap(int         value,
                      long long   other_value)
{
}

void lone_reference_gap(int    &value)
{
}
