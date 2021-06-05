void func()
{
    auto deleter = []( void* ptr ){
                       free( ptr );
                   };
}

const auto compare = [] (const auto i, const auto j)
                     {
                         return i >= j;
                     };
