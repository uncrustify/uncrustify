// #pragma asm - test nominal case
#pragma asm
    XREF     _my_var
    PUSH     #LOW(my_func)
    PUSH     _my_var+0
    RETF
App_CallEnd:
#pragma endasm

int main()
{
   int a         = 1;
   int very_long = 2;
}

// #pragma asm - test double space case
#pragma asm
    XREF     _my_var
    PUSH     #LOW(my_func)
    PUSH     _my_var+0
    RETF
App_CallEnd:
#pragma  endasm

int main2()
{
   int a         = 1;
   int very_long = 2;
}

// #pragma asm - test mixed tab/space case
#pragma asm
    XREF     _my_var
    PUSH     #LOW(my_func)
    PUSH     _my_var+0
    RETF
App_CallEnd:
#pragma         endasm

int main3()
{
   int a         = 1;
   int very_long = 2;
}

// #pragma asm - test false positive
#pragma asm
    XREF     _my_var
    PUSH     #LOW(my_func)
#pragma dummyendasm
    PUSH     _my_var+0
    RETF
App_CallEnd:
#pragma endasm

int main4()
{
   int a         = 1;
   int very_long = 2;
}

// #asm - nominal case
#asm
    sll     a
    jrc     sub_1f      ; subtract $1f if A.x msb is set
    ret
sub_1f:
    xor     a,#$1f
#endasm

int main5()
{
   int a         = 1;
   int very_long = 2;
}

