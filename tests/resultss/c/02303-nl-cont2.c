
#define FOO(bar) create_a_really_long_identifier name(some_function( \
                                                          bar1 + bar2), bar3, \
                                                      bar4);

#define multilinemacro do { (x+5); } while (0); \
    printf("a multilinemacro"); \
    printf("a multilinemacro2");

int main(int argc, char *argv[])
{
    int a, b;
    a = 1; /* stupid comment \
            * b = 2; */

    return(a+b);
}

