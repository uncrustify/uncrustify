namespace ns1 {
    int getval(void);
    void baz(void);
}


int foo(int in1, int in2, float in3, double in4) {
    int a; int b; int c; int d;
    int e; int f; int g; int h;

    a = (in1 < 10) ? in2 :
        ns1::getval();

    b = (in1 < 20) ? in2 :
        int(in3 * 0.4);

    c = (in1 < 30) ? in2 :
        1.25 * in4;

    d = (in1 < 40) ? in2 :
        -3;

    e = (in1 < 50) ? in2 :
        ~3 ^ in1;

    f = (in1 < 60) ? in2 :
        !(in1 == in2);

    g = (in1 < 70) ? in2 :
        ++in1;

    h = (in1 < 80) ? in2 :
        in1 * 2;

    if ((in1 < 10) ? in2 :
            ns1::getval()) {
        ns1::baz();
    }

    if ((in1 < 20) ? in2 :
        int(in3 * 0.4)) {
        ns1::baz();
    }

    if ((in1 < 30) ? in2 :
        1.25 * in4) {
        ns1::baz();
    }

    if ((in1 < 40) ? in2 :
        -3) {
        ns1::baz();
    }

    if ((in1 < 50) ? in2 :
        ~3 ^ in1) {
        ns1::baz();
    }

    if ((in1 < 60) ? in2 :
        !(in1 == in2)) {
        ns1::baz();
    }

    if ((in1 < 70) ? in2 :
        ++in1) {
        ns1::baz();
    }

    if ((in1 < 80) ? in2 :
        in1 * 2) {
        ns1::baz();
    }

    return a+b+c+d+e+f+g+h;
}
