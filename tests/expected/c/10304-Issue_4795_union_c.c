union U
{
    int           a;
    void          (*cb)(int);
    unsigned long bb;
    int           (*cb2)(void);
    MY_DECL(x);
    float         c;
    FOO(bar, 1);
    char *        name;
};
