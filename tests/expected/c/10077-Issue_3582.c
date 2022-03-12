#include <stdio.h>

int main(void)
{
    #if A
  {
      printf("A enabled\n");
  }
    #endif
    #if B
    {
        printf("B enabled\n");
    }
    #endif
    #if C
      {
          printf("C enabled\n");
      }
    #endif
    #if D
        {
            printf("D enabled\n");
        }
    #endif
    #if E
          {
              printf("E enabled\n");
          }
    #endif

    #if A
        printf("A enabled\n");
    #endif
    #if B
        printf("B enabled\n");
    #endif
    #if C
        printf("C enabled\n");
    #endif
    #if D
        printf("D enabled\n");
    #endif
    #if E
        printf("E enabled\n");
    #endif

    return 0;
}
