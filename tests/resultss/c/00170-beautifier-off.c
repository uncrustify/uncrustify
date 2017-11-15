

#define FOO(bar) create_a_really_long_identifier name(some_function(bar1 + bar2), bar3, bar4);

/* *INDENT-OFF* */
   int foo[] = {
      1,   3,   5,
      3,   5,   7,
      5,   7,   9,
   };
/* *INDENT-ON* */

#define multilinemacro do { (x+5); } while (0); \
	printf("a multilinemacro"); \
	printf("a multilinemacro2");

int main(int argc, char *argv[])
{
/* *INDENT-OFF* */

   int a, b;
a = 1; // stupid comment \
b = 2;

/* *INDENT-ON* */
	return(a+b);
}

/* *INDENT-OFF* */
int a;
/* *INDENT-ON* */

