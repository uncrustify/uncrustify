#include <stdio.h>

int main(int argc, char *argv[])
{
    int a = 1;
    int very_long = 2;
    printf("Hello, World!\n");
    printf("a is %d and very_long is %d\n",a,very_long);
    return 0;
}

#pragma asm
    XREF     _my_var

    PUSH     #LOW(my_func)
    PUSH     #HIGH(my_func)

#pragma dummyendasm

    PUSH     _my_var+2
    PUSH     _my_var+1
    PUSH     _my_var+0
    RETF
    App_CallEnd:
#pragma endasm

int main2(int argc, char *argv[])
{
    int a = 1;
    int very_long = 2;
    printf("Hello, World!\n");
    printf("a is %d and very_long is %d\n",a,very_long);
    return 0;
}

#pragma asm
    XREF     _my_var

    PUSH     #LOW(my_func)
    PUSH     #HIGH(my_func)

#pragma dummyendasm

    PUSH     _my_var+2
    PUSH     _my_var+1
    PUSH     _my_var+0
    RETF
    App_CallEnd:
#pragma  endasm

int main3(int argc, char *argv[])
{
    int a = 1;
    int very_long = 2;
    printf("Hello, World!\n");
    printf("a is %d and very_long is %d\n",a,very_long);
    return 0;
}

#pragma asm
    XREF     _my_var

    PUSH     #LOW(my_func)
    PUSH     #HIGH(my_func)

#pragma dummyendasm

    PUSH     _my_var+2
    PUSH     _my_var+1
    PUSH     _my_var+0
    RETF
    App_CallEnd:
#pragma	 	endasm

int main3(int argc, char *argv[])
{
    int a = 1;
    int very_long = 2;
    printf("Hello, World!\n");
    printf("a is %d and very_long is %d\n",a,very_long);
    return 0;
}

#asm
    sll     a           ; 3
    jrc     sub_1f      ; 3 subtract $1f if A.x has a degree 8
    ret
sub_1f:
    xor     a,#$1f      ; 2
#endasm

int main3(int argc, char *argv[])
{
    int a = 1;
    int very_long = 2;
    printf("Hello, World!\n");
    printf("a is %d and very_long is %d\n",a,very_long);
    return 0;
}

