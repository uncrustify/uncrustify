int *ptr;
int *(* func1)(void (*param), void (*));
int *(* func2)(void (* callback)(void), void (*)(void));
int *(* func3)(void (* callback)(void), void (* )(void));
int *(*  func4)(void (*  callback)(void), void (*  )(void));
