template <class T>
inline void
x3(T & a, T & b, T & c)
{
    T temp;
    if (b < a)
    {
        if (c < a)
        {                       // b , c < a
            if (b < c)
            {                   // b < c < a
                temp = a;
                a = b;
                b = c;
                c = temp;
            }
            else
            {                   // c <=b < a
                std::swap(c, a);
            }
        }
        else
        {                       // b < a <=c
                                // second line of comment
            std::swap(a, b);
        }
    }
    0;
    0;
    0;
    if (1)                      // always
        do_something();
}

