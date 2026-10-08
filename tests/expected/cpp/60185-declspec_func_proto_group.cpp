// Issue #4777: a '__declspec(...)'-prefixed function prototype must not be
// mistaken for the end of a prototype "group" -- Foo/Bar/Baz here are all
// ordinary prototypes and must stay together with no blank lines between
// them, regardless of the '__declspec(...)' prefix on the middle one.
class Win
{
public:
void Foo();
__declspec(dllexport) void Bar();
void Baz();

};

// Same root cause, different prefix: a 'template<...>' clause ahead of a
// member prototype must not be mistaken for the end of the group either.
class Container
{
public:
void Foo();
template<typename T>
void Bar(T value);
void Baz();

};
