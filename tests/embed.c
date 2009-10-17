

#include <stdio.h>

static const char *config_file = "config/ben.cfg";
static const char *source_file = "input/c/bugs.c";


int main(int argc, char *argv[])
{
   FILE *pf;

   char cmd[1024];
   char buf[1024];
   int  len;

   snprintf(cmd, sizeof(cmd),"uncrustify -q -c %s -f %s", config_file, source_file);

   /**
    * NOTE: to feed the data into the stdin, you'll have to use posix_spawn().
    * Refer to posix_spawn_file_actions_addopen().
    * However, it is usually OK to just to create a temporary file and use
    * that as the input.
    */

   pf = popen(cmd, "r");

   if (pf == NULL)
   {
      fprintf(stderr, "Failed to run command '%s'\n", cmd);
   }
   else
   {
      printf("--== START OF FILE ==--\n", buf);
      while (fgets(buf, sizeof(buf), pf) != NULL)
      {
         /* capture the output here */
         printf("%s", buf);
      }
      printf("\n--== END OF FILE ==--\n", buf);

      pclose(pf);
   }
   return 0;
}
