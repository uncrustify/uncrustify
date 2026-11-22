/* A qualifier after the '*' of a function pointer does not make it a call. */
void register_callback(void (*const callback)(void)) {}
void volatile_callback(int (*volatile callback)(int)) {}
void cv_callback(int (*const volatile callback)(int)) {}
void mixed(int x, void (*const callback)(void), int y) {}
void plain_callback(void (*callback)(void)) {}

void (*const global_callback)(void) = 0;

struct Table
{
    void (*const handler)(int);
};

int main(void)
{
    void (*const local_callback)(void) = 0;
    return local_callback != 0;
}
