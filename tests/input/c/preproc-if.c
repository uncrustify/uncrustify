
int main()
{
int a;
#ifndef SOMEDEF
int b;
#endif /* SOMEDEF */

if (a)
{
}
#ifndef SOMEDEF
else if (b)
{
}
#endif /* SOMEDEF */

#ifdef FOO
do
{
Foo();
}
#endif
while(Loop--);
}


