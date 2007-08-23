// When constructing an object there should not be any space between the & and the variable name:

MyClass my1(foo,& bar);
MyClass my2(foo, bar);
MyClass my3(foo, bar + 3);
MyClass my4(42);
MyClass my5(foo(), bar);
MyClass my6(int foo, int& bar);
MyClass my7(const int foo, int& bar);


//When using references inside of casts there is also an additional space after the &:

MyClass& myInst = static_cast<MyClass& >(otherInst);


// When using the qt-specific signals and slots the pointer star is separated from the type with a space:

connect(&mapper, SIGNAL(mapped(QWidget *)), this, SLOT(onSomeEvent(QWidget*)));

extern int select(int __nfds, fd_set * __restrict __readfds,
                  fd_set  *  __restrict  __writefds,
                  fd_set *  __restrict  __exceptfds,
                   struct  timeval * __restrict __timeout);

