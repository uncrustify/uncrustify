class A : public B
{
    A& operator=(const A& other)
    {
        if (this == &other) return *this;
        B::operator=(other);
        if (this == &other) return *this;
        B::opera(other);
        copy(other);
        return *this;
    }
};

