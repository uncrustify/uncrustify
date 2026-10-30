enum class Test {
    SIGNAL,
    SLOT,
};

int main() {
    return Test::SIGNAL;
}

int add(int a,int b){
    return a+b;
}

void f(Widget *w){
    add(1,2);
}
