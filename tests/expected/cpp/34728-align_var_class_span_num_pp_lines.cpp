// Test: align_var_class_span_num_pp_lines
// With span=1 and varying num_pp_lines settings.

class TestClass
{
public:
int               x;
unsigned long int y;
#ifdef DEBUG
double            z;
#endif
#ifdef EXTRA
float             w;
#endif
#ifdef MORE
#ifdef NESTED
short             s;
#endif
#endif
};
