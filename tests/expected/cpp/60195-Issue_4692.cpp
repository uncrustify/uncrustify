template <typename ... Args>
void foo(bool               flag,
         unsigned long long expected,
         Args&& ...         args);

template <typename Arg>
void bar(bool               flag,
         unsigned long long expected,
         Arg&&              arg);

template <typename ... Args>
struct S {
    void member(bool               flag,
                unsigned long long expected,
                Args&&...          args);
};
