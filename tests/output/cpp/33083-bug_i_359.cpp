int main()
{
    int foo = 42;
    switch (foo) {
    case 1:
        std::cout << "1" << std::endl;
        break;
    case 2:
        std::cout << "2" << std::endl;
        break;
    default:
        std::cout << "Neither 1 nor 2." << std::endl;
    }
}
