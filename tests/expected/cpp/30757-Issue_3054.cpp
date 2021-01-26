void func()
{
  parallel_for(0, 100, [&](const int i){
    const std::vector<int> values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
    return values[i];
  });
}
