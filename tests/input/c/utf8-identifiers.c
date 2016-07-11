void FooUtf8Сhar(void) // C is encoded in UTF-8
{
}

struct テスト         // Japanese 'test'
{
    void トスト() {}  // Japanese 'toast'
};

int main() {
    テスト パン;        // Japanese パン 'bread'
    パン.トスト();
}
