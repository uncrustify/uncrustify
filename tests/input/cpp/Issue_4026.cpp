class foo {
public:
    int var;
 
    foo(int x) { var = x; }
};

int main() 
{
    int a = 2;
    a++;

    foo f(3);
    f.var++;
}
