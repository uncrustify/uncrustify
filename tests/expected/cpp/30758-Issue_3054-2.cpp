void func()
{
  parallel_for(0, 100,
               [&](int aaaaaa, int bbbbbbb, int ccccccc, int ddddddd,
                   const int eee){
    // do something
    return a;
  });
}
