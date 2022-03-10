#include <stdio.h>

int main(int argc, char **argv)
{
    switch (argc)
    {
    case -1:
  {
  printf("Negative args shouldn't be possible\n");
  break;
  }
    case 0:
    {
    printf("Zero args can happen but shouldn't\n");
    break;
    }
    case 1:
      {
      printf("One arg\n");
      break;
      }
    case 2:
        {
        printf("Two args\n");
        break;
        }
    default:
          {
          printf("%i args\n", argc);
          break;
          }
    }

    return 0;
}
