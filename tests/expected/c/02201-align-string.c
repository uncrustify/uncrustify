
// note - set threshold to three
void foo(void)
{
   printf("This is the first line\n"
          "And this is the second.\n");

   fprintf(stderr, "This is the first line\n"
                   "And this is the second.\n");

   fprintf(stderr, "Format string: %s", "This is the first line\n"
                                        "And this is the second.\n");
}

