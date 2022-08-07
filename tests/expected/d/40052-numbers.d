/* This file goes over all the various number formats for D */

int foo(int bar) {
    int a;

    /*
     *  Interger Literals
     */

    /* Suffixes */
    a = 0L + 123U + 345u + 456Lu + 567LU + 678UL + 789_uL;

    /* Decimal */
    a = 0 + 123456 + 123_456 + 1_2_3_4_5_6;

    /* Binary */
    a = 0b1010101 + 0B1001;

    /* Octal */
    a = 01234567_ + 07_6_5_2;

    /* Hexadecimal */
    a = 0x1234567890abcdefABCDEF_ + 0X7_6_5_2;

    float  b;
    real   c;
    ifloat d;
    ireal  e;

    /* Floats: float [FloatSuffix] [ImaginarySuffix] */

    /* HexFloat */
    b = 0xabc.defp-1024 + 0x.defP-64 + 0x123p+32 + 0x123P+16 + 0x123p1024;
    d = 0x123p45 + 0x234.fi + 0. + .3f;
    e = 3 + 5i;
    e = 3.4 + 5.6i;
}

/* test '..' ranges */
void main() {
    char[] c = "kkkkkkkkkkkkkkkkkkkkk";

    writefln("%s", c[2..3]);
}
