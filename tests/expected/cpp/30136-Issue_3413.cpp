namespace
{
struct S { void f() {} };
}

void FuncCrash(int a =       {}) { }
void FuncCrash(int b = int   {}) { }
void FuncCrash(int b = int(0)) { }
void FuncCrash(int b = double{0}) { }
void FuncCrash(int b = 0) { }
