// A qualifier after the '*' of a function pointer does not make it a call.
void register_callback(void (*const callback)(void)) {
}
void volatile_callback(int (*volatile callback)(int)) {
}
void cv_callback(int (*const volatile callback)(int)) {
}
void spaced_callback(void (* const callback)(void)) {
}
void two_callbacks(void (*const first)(void), int (*const second)(int)) {
}
void mixed(int x, void (*const callback)(void), int y) {
}
void with_return_pointer(int *(*const callback)(void)) {
}
void unnamed(void (*const)(void)) {
}
void plain_callback(void (*callback)(void)) {
}

struct Foo { int method(int); };
void member_callback(int (Foo::*const method)(int)) {
}

void (*const global_callback)(void) = nullptr;
const char *(*const global_lookup)(int, int) = nullptr;

struct Table
{
    void (*const handler)(int);
};

int main()
{
    void (*const local_callback)(void) = nullptr;
    local_callback();
    return 0;
}
