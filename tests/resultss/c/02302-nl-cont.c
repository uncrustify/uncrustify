
#define FOO(bar) create_a_really_long_identifier name(some_function( \
                                                          bar1 + bar2), bar3, \
                                                      bar4);

#define VNV_RECORD_CYCLES(m) do { \
        uint16_t cyc_out = ((uint16_t )TMR4) - cyc_in; \
        if (cyc_out < vnv_ticks[m].min) vnv_ticks[m].min = cyc_out; \
        if (cyc_out > vnv_ticks[m].max) vnv_ticks[m].max = cyc_out; \
} while (0)

#define multilinemacro do { (x+5); } while (0); \
    printf("a multilinemacro"); \
    printf("a multilinemacro2");

int main(int argc, char *argv[])
{
    int a, b;
    a = 1; /* stupid comment \\ */
    b = 2;

    return(a+b);
}
