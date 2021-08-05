#include <stdio.h>

void hello(void)
{
        printf("Hello world!\n");
}

void (*get_hello(void))(void)
{
        return hello;
}

int main()
{
        (get_hello())();
        return 0;
}
