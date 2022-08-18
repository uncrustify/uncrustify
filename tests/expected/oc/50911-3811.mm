int main()
{
  test([](enum TestEnum lhs) {
    return lhs.first < rhs;
  });
}
