class foo {
public:
    bool var;
 
    foo(bool x) { var = x; }
 
    bool get()
    {
        return var;
    }
};


int main() {
    foo f(false);

    const int b = true ? 2 : 3;

    const int a = !f.get() ? 2 : 3;
}
